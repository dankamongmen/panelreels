#include <errno.h>
#include <panel.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "outcurses.h"

// Tablets are the toplevel entitites within a panelreel. Each corresponds to
// a single, distinct PANEL.
typedef struct tablet {
  PANEL* p;                    // visible panel, NULL when offscreen
  struct tablet* next;
  struct tablet* prev;
  tabletcb cbfxn;              // application callback to draw tablet
  void* curry;                 // application data provided to cbfxn
} tablet;

// The visible screen can be reconstructed from three things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (derived from focused window)
//  * the list of tablets (available from the focused tablet)
typedef struct panelreel {
  PANEL* p;                // PANEL this panelreel occupies, under tablets
  panelreel_options popts; // copied in panelreel_create()
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused), which might be anywhere on the screen (but is always visible).
  int efd;                 // eventfd, signaled in panelreel_touch() if >= 0
  tablet* tablets;
  // these values could all be derived at any time, but keeping them computed
  // makes other things easier, or saves us time (at the cost of complexity).
  int tabletcount;         // could be derived, but we keep it o(1)
  // last direction in which we moved. positive if we moved down ("next"),
  // negative if we moved up ("prev"), 0 for non-linear operation. we start
  // drawing unfocused tablets opposite the direction of our last movement, so
  // that movement in an unfilled reel doesn't reorient our tablets.
  int last_traveled_direction;
  // are all of our tablets currently visible? our arrangement algorithm works
  // differently when the reel is not completely filled. ideally we'd unite the
  // two modes, but for now, check this bool and take one of two paths.
  bool all_visible;
} panelreel;

// Returns the starting coordinates (relative to the screen) of the specified
// window, and its length. End is (begx + lenx - 1, begy + leny - 1).
static inline void
window_coordinates(const WINDOW* w, int* begy, int* begx, int* leny, int* lenx){
  *begx = getbegx(w);
  *begy = getbegy(w);
  *lenx = getmaxx(w);
  *leny = getmaxy(w);
}

// These ought be part of ncurses, probably
static const cchar_t WACS_R_ULCORNER = { .attr = 0, .chars = L"╭", };
static const cchar_t WACS_R_URCORNER = { .attr = 0, .chars = L"╮", };
static const cchar_t WACS_R_LLCORNER = { .attr = 0, .chars = L"╰", };
static const cchar_t WACS_R_LRCORNER = { .attr = 0, .chars = L"╯", };

// bchrs: 6-element array of wide border characters + attributes FIXME
static int
draw_borders(WINDOW* w, unsigned nobordermask, attr_t attr, int pair,
             bool cliphead, bool clipfoot){
  wattr_set(w, attr, 0, &pair);
  int begx, begy, lenx, leny;
  int ret = OK;
  window_coordinates(w, &begy, &begx, &leny, &lenx);
  begx = 0;
  begy = 0;
  int maxx = begx + lenx - 1;
  int maxy = begy + leny - 1;
/*fprintf(stderr, "drawing borders %d/%d->%d/%d, mask: %04x, clipping: %c%c\n",
        begx, begy, maxx, maxy, nobordermask,
        cliphead ? 'T' : 't', clipfoot ? 'F' : 'f');*/
  if(!cliphead){
    // lenx - begx + 1 is the number of columns we have, but drop 2 due to
    // corners. we thus want lenx - begx - 1 horizontal lines.
    if(!(nobordermask & BORDERMASK_TOP)){
      ret |= mvwadd_wch(w, begy, begx, &WACS_R_ULCORNER);
      ret |= whline_set(w, WACS_HLINE, lenx - 2);
      ret |= mvwadd_wch(w, begy, maxx, &WACS_R_URCORNER);
    }else{
      if(!(nobordermask & BORDERMASK_LEFT)){
        ret |= mvwadd_wch(w, begy, begx, &WACS_R_ULCORNER);
      }
      if(!(nobordermask & BORDERMASK_RIGHT)){
        ret |= mvwadd_wch(w, begy, maxx, &WACS_R_URCORNER);
      }
    }
  }
  int y;
  for(y = begy + !cliphead ; y < maxy + !!clipfoot ; ++y){
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, y, begx, WACS_VLINE);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      ret |= mvwadd_wch(w, y, maxx, WACS_VLINE);
    }
  }
  if(!clipfoot){
    if(!(nobordermask & BORDERMASK_BOTTOM)){
      ret |= mvwadd_wch(w, maxy, begx, &WACS_R_LLCORNER);
      ret |= whline_set(w, WACS_HLINE, lenx - 2);
      mvwadd_wch(w, maxy, maxx, &WACS_R_LRCORNER); // always errors
    }else{
      if(!(nobordermask & BORDERMASK_LEFT)){
        ret |= mvwadd_wch(w, maxy, begx, &WACS_R_LLCORNER);
      }
      if(!(nobordermask & BORDERMASK_RIGHT)){
        // mvwadd_wch returns error if we print to the lowermost+rightmost
        // character cell. maybe we can make this go away with scrolling controls
        // at setup? until then, don't check for error here FIXME.
        mvwadd_wch(w, maxy, maxx, &WACS_R_LRCORNER);
      }
    }
  }
// fprintf(stderr, "||--borders %d %d %d %d clip: %c%c ret: %d\n",
//    begx, begy, maxx, maxy, cliphead ? 'y' : 'n', clipfoot ? 'y' : 'n', ret);
  return ret;
}

// Draws the border (if one should be drawn) around the panelreel, and enforces
// any provided restrictions on visible window size.
static int
draw_panelreel_borders(const panelreel* pr){
  WINDOW* w = panel_window(pr->p);
  int begx, begy;
  int maxx, maxy;
  getbegyx(w, begy, begx);
  getmaxyx(w, maxy, maxx);
  assert(begy >= 0 && begx >= 0);
  assert(maxy >= 0 && maxx >= 0);
  --maxx; // last column we can safely write to
  --maxy; // last line we can safely write to
  if(begx >= maxx || maxx - begx + 1 < pr->popts.min_supported_rows){
    return 0; // no room
  }
  if(begy >= maxy || maxy - begy + 1 < pr->popts.min_supported_cols){
    return 0; // no room
  }
  return draw_borders(w, pr->popts.bordermask, pr->popts.borderattr,
                      pr->popts.borderpair, false, false);
}

// Calculate the starting and ending coordinates available for occupation by
// the tablet, relative to the panelreel's WINDOW. Returns non-zero if the
// tablet cannot be made visible as specified. If this is the focused tablet
// (direction == 0), it can take the entire reel -- frontiery is only a
// suggestion in this case -- so give it the full breadth.
static int
tablet_columns(const panelreel* pr, int* begx, int* begy, int* lenx, int* leny,
               int frontiery, int direction){
  WINDOW* w = panel_window(pr->p);
  window_coordinates(w, begy, begx, leny, lenx);
  int maxy = *leny + *begy - 1;
  int begindraw = *begy + !(pr->popts.bordermask & BORDERMASK_TOP);
  // FIXME i think this fails to account for an absent panelreel bottom?
  int enddraw = maxy - !(pr->popts.bordermask & BORDERMASK_TOP);
  if(direction){
    if(frontiery < begindraw){
      return -1;
    }
    if(frontiery > enddraw){
  // fprintf(stderr, "FRONTIER: %d ENDDRAW: %d\n", frontiery, enddraw);
      return -1;
    }
  }
  // account for the panelreel borders
  if(direction <= 0 && !(pr->popts.bordermask & BORDERMASK_TOP)){
    ++*begy;
    --*leny;
  }
  if(direction >= 0 && !(pr->popts.bordermask & BORDERMASK_BOTTOM)){
    --*leny;
  }
  if(!(pr->popts.bordermask & BORDERMASK_LEFT)){
    ++*begx;
    --*lenx;
  }
  if(!(pr->popts.bordermask & BORDERMASK_RIGHT)){
    --*lenx;
  }
  // at this point, our coordinates describe the largest possible tablet for
  // this panelreel. this is the correct solution for the focused tablet. other
  // tablets can only grow in one of two directions, so tighten them up.
  if(direction > 0){
    *leny -= (frontiery - *begy);
    *begy = frontiery;
  }else if(direction < 0){
    *leny = frontiery - *begy + 1;
  }
  return 0;
}

// Draw the specified tablet, if possible. A direction less than 0 means we're
// laying out towards the top. Greater than zero means towards the bottom. 0
// means this is the focused tablet, always the first one to be drawn.
// frontiery is the line on which we're placing the tablet (in the case of the
// focused window, this is only an ideal, subject to change). For direction
// greater than or equal to 0, it's the top line of the tablet. For direction
// less than 0, it's the bottom line. Gives the tablet all possible space to
// work with (i.e. up to the edge we're approaching, or the entire panel for
// the focused tablet). If the callback uses less space, shrinks the panel back
// down before displaying it. Destroys any panel if it ought be hidden.
// Returns 0 if the tablet was able to be wholly rendered, non-zero otherwise.
static int
panelreel_draw_tablet(const panelreel* pr, tablet* t, int frontiery,
                      int direction){
  int lenx, leny, begy, begx;
  WINDOW* w;
  PANEL* fp = t->p;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, direction)){
//fprintf(stderr, "no room: %p:%p base %d/%d len %d/%d\n", t, fp, begx, begy, lenx, leny);
// fprintf(stderr, "FRONTIER DONE!!!!!!\n");
    if(fp){
// fprintf(stderr, "HIDING %p at frontier %d (dir %d) with %d\n", t, frontiery, direction, leny);
      w = panel_window(fp);
      del_panel(fp);
      delwin(w);
      t->p = NULL;
      update_panels();
    }
    return -1;
  }
// fprintf(stderr, "tplacement: %p:%p base %d/%d len %d/%d\n", t, fp, begx, begy, lenx, leny);
// fprintf(stderr, "DRAWING %p at frontier %d (dir %d) with %d\n", t, frontiery, direction, leny);
  if(fp == NULL){ // create a panel for the tablet
    w = newwin(leny + 1, lenx, begy, begx);
    if(w == NULL){
      return -1;
    }
    t->p = new_panel(w);
    if((fp = t->p) == NULL){
      delwin(w);
      return -1;
    }
  }else{
    w = panel_window(fp);
    int trueby = getbegy(w);
    int truey, truex;
    getmaxyx(w, truey, truex);
    if(truey != leny){
// fprintf(stderr, "RESIZE TRUEY: %d BEGY: %d LENY: %d\n", truey, begy, leny);
      if(wresize(w, leny, truex)){
        return -1;
      }
      getmaxyx(w, truey, truex);
    }
    if(begy != trueby){
      if(move_panel(fp, begy, begx)){
        return -1;
      }
    }
  }
  wresize(w, leny, lenx);
  bool cliphead = false;
  bool clipfoot = false;
  // We pass the coordinates in which the callback may freely write. That's
  // the full width (minus tablet borders), and the full range of open space
  // in the direction we're moving. We're not passing *lenghts* to the callback,
  // but *coordinates* within the window--everywhere save tabletborders.
  int cby = 0, cbx = 0, cbmaxy = leny, cbmaxx = lenx;
  --cbmaxy;
  --cbmaxx;
  // If we're drawing up, we'll always have a bottom border unless it's masked
  if(direction < 0 && !(pr->popts.tabletmask & BORDERMASK_BOTTOM)){
    --cbmaxy;
  }
  // If we're drawing down, we'll always have a top border unless it's masked
  if(direction >= 0 && !(pr->popts.tabletmask & BORDERMASK_TOP)){
    ++cby;
  }
  // Adjust the x-bounds for side borders, which we always have if unmasked
  cbmaxx -= !(pr->popts.tabletmask & BORDERMASK_RIGHT);
  cbx += !(pr->popts.tabletmask & BORDERMASK_LEFT);
  bool cbdir = direction < 0 ? true : false;
// fprintf(stderr, "calling! lenx/leny: %d/%d cbx/cby: %d/%d cbmaxx/cbmaxy: %d/%d dir: %d\n",
//    lenx, leny, cbx, cby, cbmaxx, cbmaxy, direction);
  int ll = t->cbfxn(fp, cbx, cby, cbmaxx, cbmaxy, cbdir, t->curry);
//fprintf(stderr, "RETURNRETURNRETURN %p %d (%d, %d, %d) DIR %d\n",
//        t, ll, cby, cbmaxy, leny, direction);
  if(ll != leny){
    if(ll == leny - 1){ // only has one border visible (partially off-screen)
      ++ll; // account for that border
      wresize(w, ll, lenx);
      if(direction < 0){
        cliphead = true;
        move_panel(fp, begy + leny - ll, begx);
// fprintf(stderr, "MOVEDOWN CLIPPED RESIZED (-1) from %d to %d\n", leny, ll);
      }else{
        clipfoot = true;
// fprintf(stderr, "RESIZED (-1) from %d to %d\n", leny, ll);
      }
    }else{ // both borders are visible
      ll += 2; // account for both borders
// fprintf(stderr, "RESIZING (-2) from %d to %d\n", leny, ll);
      wresize(w, ll, lenx);
      if(direction < 0){
// fprintf(stderr, "MOVEDOWN UNCLIPPED (skip %d)\n", leny - ll);
        if(move_panel(fp, begy + leny - ll, begx)){
          return -1;
        }
      }
    }
    // The focused tablet will have been resized properly above, but it might
    // be out of position (the focused tablet ought move as little as possible). 
    // Move it back to the frontier, or the nearest line above if it has grown.
    if(direction == 0){
      if(leny - frontiery + 1 < ll){
//fprintf(stderr, "frontieryIZING ADJ %d %d %d %d NEW %d\n", cbmaxy, leny,
//         frontiery, ll, frontiery - ll + 1);
        frontiery = leny - ll + 1 + getbegy(panel_window(pr->p));
      }
      if(move_panel(fp, frontiery, begx)){
        return -1;
      }
    }
  }
  draw_borders(w, pr->popts.tabletmask,
                direction == 0 ? pr->popts.focusedattr : pr->popts.tabletattr,
                direction == 0 ? pr->popts.focusedpair : pr->popts.tabletpair,
                cliphead, clipfoot);
  return cliphead || clipfoot;
}

// draw and size the focused tablet, which must exist (pr->tablets may not be
// NULL). it can occupy the entire panelreel.
static int
draw_focused_tablet(const panelreel* pr){
  int pbegy, pbegx, plenx, pleny; // panelreel window coordinates
  window_coordinates(panel_window(pr->p), &pbegy, &pbegx, &pleny, &plenx);
  int fulcrum;
  if(pr->tablets->p == NULL){
    if(pr->last_traveled_direction >= 0){
      fulcrum = pleny + pbegy - !(pr->popts.bordermask & BORDERMASK_BOTTOM);
    }else{
      fulcrum = pbegy + !(pr->popts.bordermask & BORDERMASK_TOP);
    }
  }else{ // focused was already present. want to stay where we are, if possible
    fulcrum = getbegy(panel_window(pr->tablets->p));
    // FIXME ugh can't we just remember the previous fulcrum?
    if(pr->last_traveled_direction > 0){
      if(pr->tablets->prev->p){
        if(fulcrum < getbegy(panel_window(pr->tablets->prev->p))){
          fulcrum = pleny + pbegy - !(pr->popts.bordermask & BORDERMASK_BOTTOM);
        }
      }
    }else if(pr->last_traveled_direction < 0){
      if(pr->tablets->next->p){
        if(fulcrum > getbegy(panel_window(pr->tablets->next->p))){
          fulcrum = pbegy + !(pr->popts.bordermask & BORDERMASK_TOP);
        }
      }
    }
  }
//fprintf(stderr, "PR dims: %d/%d + %d/%d fulcrum: %d\n", pbegy, pbegx, pleny, plenx, fulcrum);
  panelreel_draw_tablet(pr, pr->tablets, fulcrum, 0);
  return 0;
}

// move down below the focused tablet, filling up the reel to the bottom.
// returns the last tablet drawn.
static tablet*
draw_following_tablets(const panelreel* pr, const tablet* otherend){
  int wmaxy, wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  tablet* working = pr->tablets;
  int frontiery;
  // move down past the focused tablet, filling up the reel to the bottom
  while(working->next != otherend || otherend->p == NULL){
    window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
    wmaxy = wbegy + wleny - 1;
    frontiery = wmaxy + 2;
//fprintf(stderr, "EASTBOUND AND DOWN: %d %d\n", frontiery, wmaxy + 2);
    working = working->next;
    panelreel_draw_tablet(pr, working, frontiery, 1);
    if(working->p == NULL){ // FIXME might be more to hide
      break;
    }
  }
  // FIXME keep going forward, hiding those no longer visible
  return working;
}

// move up above the focused tablet, filling up the reel to the top.
// returns the last tablet drawn.
static tablet*
draw_previous_tablets(const panelreel* pr, const tablet* otherend){
  int wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  tablet* upworking = pr->tablets;
  int frontiery;
  while(upworking->prev != otherend || otherend->p == NULL){
    window_coordinates(panel_window(upworking->p), &wbegy, &wbegx, &wleny, &wlenx);
    frontiery = wbegy - 2;
//fprintf(stderr, "MOVIN' ON UP: %d %d\n", frontiery, wbegy - 2);
    upworking = upworking->prev;
    panelreel_draw_tablet(pr, upworking, frontiery, -1);
    if(upworking->p){
      window_coordinates(panel_window(upworking->p), &wbegy, &wbegx, &wleny, &wlenx);
//fprintf(stderr, "new up coords: %d/%d + %d/%d, %d\n", wbegy, wbegx, wleny, wlenx, frontiery);
      frontiery = wbegy - 2;
    }else{
      break;
    }
    if(upworking == otherend){
      otherend = otherend->prev;
    }
  }
  // FIXME keep going backwards, hiding those no longer visible
  return upworking;
}

// all tablets must be visible (valid ->p), and at least one tablet must exist
static tablet*
find_topmost(panelreel* pr){
  tablet* t = pr->tablets;
  int curline = getbegy(panel_window(t->p));
  int trialline = getbegy(panel_window(t->prev->p));
  while(trialline < curline){
    t = t->prev;
    curline = trialline;
    trialline = getbegy(panel_window(t->prev->p));
  }
// fprintf(stderr, "topmost: %p @ %d\n", t, curline);
  return t;
}

// all the tablets are believed to be wholly visible. in this case, we only want
// to fill up the necessary top rows, even if it means moving everything up at
// the end. large gaps should always be at the bottom to avoid ui discontinuity.
// this must only be called if we actually have at least one tablet. note that
// as a result of this function, we might not longer all be wholly visible.
// good god almighty, this is some fucking garbage.
static int
panelreel_arrange_denormalized(panelreel* pr){
  // we'll need the starting line of the tablet which just lost focus, and the
  // starting line of the tablet which just gained focus.
  int fromline, nowline;
  nowline = getbegy(panel_window(pr->tablets->p));
  // we've moved to the next or previous tablet. either we were not at the end,
  // in which case we can just move the focus, or we were at the end, in which
  // case we need bring the target tablet to our end, and draw in the direction
  // opposite travel (a single tablet is a trivial case of the latter case).
  // how do we know whether we were at the end? if the new line is not in the
  // direction of movement relative to the old one, of course!
  tablet* topmost = find_topmost(pr);
  int wbegy, wbegx, wleny, wlenx;
  window_coordinates(panel_window(pr->p), &wbegy, &wbegx, &wleny, &wlenx);
  int frontiery = wbegy + !(pr->popts.bordermask & BORDERMASK_TOP);
  if(pr->last_traveled_direction >= 0){
    fromline = getbegy(panel_window(pr->tablets->prev->p));
    if(fromline > nowline){ // keep the order we had
      topmost = topmost->next;
    }
  }else{
    fromline = getbegy(panel_window(pr->tablets->next->p));
    if(fromline < nowline){ // keep the order we had
      topmost = topmost->prev;
    }
  }
// fprintf(stderr, "gotta draw 'em all FROM: %d NOW: %d!\n", fromline, nowline);
  tablet* t = topmost;
  do{
    int broken;
    if(t == pr->tablets){
      broken = panelreel_draw_tablet(pr, t, frontiery, 0);
    }else{
      broken = panelreel_draw_tablet(pr, t, frontiery, 1);
    }
    if(t->p == NULL || broken){
      pr->all_visible = false;
      break;
    }
    frontiery = getmaxy(panel_window(t->p)) + getbegy(panel_window(t->p)) + 1;
  }while((t = t->next) != topmost);
  return 0;
}

// Arrange the panels, starting with the focused window, wherever it may be.
// If necessary, resize it to the full size of the reel--focus has its
// privileges. We then work in the opposite direction of travel, filling out
// the reel above and below. If we moved down to get here, do the tablets above
// first. If we moved up, do the tablets below. This ensures tablets stay in
// place relative to the new focus; they could otherwise pivot around the new
// focus, if we're not filling out the reel.
//
// This can still leave a gap plus a partially-onscreen tablet FIXME
static int
panelreel_arrange(panelreel* pr){
  tablet* focused = pr->tablets;
  if(focused == NULL){
    return 0; // if none are focused, none exist
  }
  // FIXME we special-cased this because i'm dumb and couldn't think of a more
  // elegant way to do this. we keep 'all_visible' as boolean state to avoid
  // having to do an o(n) iteration each round, but this is still grotesque, and
  // feels fragile...
  if(pr->all_visible){
    return panelreel_arrange_denormalized(pr);
  }
  draw_focused_tablet(pr);
  tablet* otherend = focused;
  if(pr->last_traveled_direction >= 0){
    otherend = draw_previous_tablets(pr, otherend);
    otherend = draw_following_tablets(pr, otherend);
  }else{
    otherend = draw_following_tablets(pr, otherend);
    otherend = draw_previous_tablets(pr, otherend);
  }
  // FIXME move them up to plug any holes in original direction?
//fprintf(stderr, "DONE ARRANGING\n");
  return 0;
}

int panelreel_redraw(panelreel* pr){
//fprintf(stderr, "--------> BEGIN REDRAW <--------\n");
  int ret = 0;
  if(draw_panelreel_borders(pr)){
    return -1; // enforces specified dimensional minima
  }
  ret |= panelreel_arrange(pr);
  update_panels();
  ret |= doupdate();
  return ret;
}

static bool
validate_panelreel_opts(WINDOW* w, const panelreel_options* popts){
  if(w == NULL){
    return false;
  }
  if(!popts->infinitescroll){
    if(popts->circular){
      return false; // can't set circular without infinitescroll
    }
  }
  const unsigned fullmask = BORDERMASK_LEFT |
                            BORDERMASK_RIGHT |
                            BORDERMASK_TOP |
                            BORDERMASK_BOTTOM;
  if(popts->bordermask > fullmask){
    return false;
  }
  if(popts->tabletmask > fullmask){
    return false;
  }
  return true;
}

panelreel* panelreel_create(WINDOW* w, const panelreel_options* popts, int efd){
  panelreel* pr;

  if(!validate_panelreel_opts(w, popts)){
    return NULL;
  }
  if((pr = malloc(sizeof(*pr))) == NULL){
    return NULL;
  }
  pr->efd = efd;
  pr->tablets = NULL;
  pr->tabletcount = 0;
  pr->all_visible = true;
  pr->last_traveled_direction = -1; // draw down after the initial tablet
  memcpy(&pr->popts, popts, sizeof(*popts));
  int maxx, maxy, wx, wy;
  getbegyx(w, wy, wx);
  getmaxyx(w, maxy, maxx);
  --maxy;
  --maxx;
  int ylen, xlen;
  ylen = maxy - popts->boff - popts->toff + 1;
  if(ylen < 0){
    ylen = maxy - popts->toff;
    if(ylen < 0){
      ylen = 0; // but this translates to a full-screen window...FIXME
    }
  }
  xlen = maxx - popts->roff - popts->loff + 1;
  if(xlen < 0){
    xlen = maxx - popts->loff;
    if(xlen < 0){
      xlen = 0; // FIXME see above...
    }
  }
  WINDOW* pw = newwin(ylen, xlen, popts->toff + wy, popts->loff + wx);
  if(pw == NULL){
    free(pr);
    return NULL;
  }
  if((pr->p = new_panel(pw)) == NULL){
    delwin(pw);
    free(pr);
    return NULL;
  }
  if(panelreel_redraw(pr)){
    del_panel(pr->p);
    delwin(pw);
    free(pr);
    return NULL;
  }
  return pr;
}

// we've just added a new tablet. it need be inserted at the correct place in
// the reel. this will naturally fall out of things if the panelreel is full; we
// can just call panelreel_redraw(). otherwise, we need make ourselves at least
// minimally visible, to satisfy the preconditions of
// panelreel_arrange_denormalized(). this function, and approach, is shit.
static tablet*
insert_new_panel(panelreel* pr, tablet* t){
  if(!pr->all_visible){
    return t;
  }
  int wbegy, wbegx, wleny, wlenx; // params of PR
  window_coordinates(panel_window(pr->p), &wbegy, &wbegx, &wleny, &wlenx);
  WINDOW* w;
  // are we the only tablet?
  int begx, begy, lenx, leny, frontiery;
  if(t->prev == t){
    frontiery = wbegy + !(pr->popts.bordermask & BORDERMASK_TOP);
    if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, 1)){
      pr->all_visible = false;
      return t;
    }
//fprintf(stderr, "newwin: %d/%d + %d/%d\n", begy, begx, leny, lenx);
    if((w = newwin(leny, lenx, begy, begx)) == NULL){
      pr->all_visible = false;
      return t;
    }
    if((t->p = new_panel(w)) == NULL){
      delwin(w);
      pr->all_visible = false;
      return t;
    }
//fprintf(stderr, "created first tablet!\n");
    return t;
  }
  // we're not the only tablet, alas.
  // our new window needs to be after our prev
  frontiery = getbegy(panel_window(t->prev->p));
  frontiery += getmaxy(panel_window(t->prev->p));
  frontiery += 2;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, 1)){
    pr->all_visible = false;
    return t;
  }
  if((w = newwin(2, lenx, begy, begx)) == NULL){
    pr->all_visible = false;
    return t;
  }
  if((t->p = new_panel(w)) == NULL){
    delwin(w);
    pr->all_visible = false;
    return t;
  }
  // FIXME push the other ones down by 4
  return t;
}

tablet* panelreel_add(panelreel* pr, tablet* after, tablet *before,
                      tabletcb cbfxn, void* opaque){
  tablet* t;
  if(after && before){
    if(after->prev != before || before->next != after){
      return NULL;
    }
  }else if(!after && !before){
    // This way, without user interaction or any specification, new tablets are
    // inserted at the "end" relative to the focus. The first one to be added
    // gets and keeps the focus. New ones will go on the bottom, until we run
    // out of space. New tablets are then created off-screen.
    before = pr->tablets;
  }
  if((t = malloc(sizeof(*t))) == NULL){
    return NULL;
  }
//fprintf(stderr, "--------->NEW TABLET %p\n", t);
  if(after){
    t->next = after->next;
    after->next = t;
    t->prev = after;
    t->next->prev = t;
  }else if(before){
    t->prev = before->prev;
    before->prev = t;
    t->next = before;
    t->prev->next = t;
  }else{ // we're the first tablet
    t->prev = t->next = t;
    pr->tablets = t;
  }
  t->cbfxn = cbfxn;
  t->curry = opaque;
  ++pr->tabletcount;
  t->p = NULL;
  // if we have room, it needs become visible immediately, in the proper place,
  // lest we invalidate the preconditions of panelreel_arrange_denormalized().
  insert_new_panel(pr, t);
  panelreel_redraw(pr); // don't return failure; tablet was still created...
  return t;
}

int panelreel_del_focused(struct panelreel* pr){
  return panelreel_del(pr, pr->tablets);
}

int panelreel_del(struct panelreel* pr, struct tablet* t){
  if(pr == NULL || t == NULL){
    return -1;
  }
  t->prev->next = t->next;
  if(pr->tablets == t){
    if((pr->tablets = t->next) == t){
      pr->tablets = NULL;
    }
  }
  t->next->prev = t->prev;
  if(t->p){
    WINDOW* w = panel_window(t->p);
    del_panel(t->p);
    delwin(w);
  }
  free(t);
  --pr->tabletcount;
  update_panels();
  panelreel_redraw(pr);
  return 0;
}

int panelreel_destroy(panelreel* preel){
  int ret = 0;
  if(preel){
    tablet* t = preel->tablets;
    while(t){
      t->prev->next = NULL;
      tablet* tmp = t->next;
      panelreel_del(preel, t);
      t = tmp;
    }
    free(preel);
  }
  return ret;
}

int panelreel_tabletcount(const panelreel* preel){
  return preel->tabletcount;
}

int panelreel_touch(panelreel* pr, tablet* t){
  (void)t; // FIXME make these more granular eventually
  int ret = 0;
  if(pr->efd >= 0){
    uint64_t val = 1;
    if(write(pr->efd, &val, sizeof(val)) != sizeof(val)){
      fprintf(stderr, "Error writing to eventfd %d (%s)\n",
              pr->efd, strerror(errno));
      ret = -1;
    }
  }
  return ret;
}

// Move to some position relative to the current position
static int
move_tablet(PANEL* p, int deltax, int deltay){
  WINDOW* w = panel_window(p);
  int oldx, oldy;
  getbegyx(w, oldy, oldx);
  int x = oldx + deltax;
  int y = oldy + deltay;
  if(move_panel(p, y, x) != OK){
    return -1;
  }
  return 0;
}

tablet* panelreel_focused(panelreel* pr){
  return pr->tablets;
}

int panelreel_move(panelreel* preel, int x, int y){
  WINDOW* w = panel_window(preel->p);
  int oldx, oldy;
  getbegyx(w, oldy, oldx);
  const int deltax = x - oldx;
  const int deltay = y - oldy;
  if(move_tablet(preel->p, deltax, deltay)){
    move_panel(preel->p, oldy, oldx);
    panelreel_redraw(preel);
    return -1;
  }
  if(preel->tablets){
    tablet* t = preel->tablets;
    do{
      if(t->p == NULL){
        break;
      }
      move_tablet(t->p, deltax, deltay);
    }while((t = t->prev) != preel->tablets);
    if(t != preel->tablets){ // don't repeat if we covered all tablets
      for(t = preel->tablets->next ; t != preel->tablets ; t = t->next){
        if(t->p == NULL){
          break;
        }
        move_tablet(t->p, deltax, deltay);
      }
    }
  }
  update_panels();
  panelreel_redraw(preel);
  return 0;
}

tablet* panelreel_next(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->next;
//fprintf(stderr, "---------------> moved to next, %p to %p <----------\n",
//        pr->tablets->prev, pr->tablets);
    pr->last_traveled_direction = 1;
  }
  panelreel_redraw(pr);
  return pr->tablets;
}

tablet* panelreel_prev(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->prev;
//fprintf(stderr, "----------------> moved to prev, %p to %p <----------\n",
//        pr->tablets->next, pr->tablets);
    pr->last_traveled_direction = -1;
  }
  panelreel_redraw(pr);
  return pr->tablets;
}

// Used for unit tests. Step through the panelreel and verify that everything
// seems to be where it ought be, considering its parent WINDOW.
int panelreel_validate(WINDOW* parent, panelreel* pr){
  PANEL* p = pr->p;
  WINDOW* w = panel_window(p);
  // First, verify geometry relative to parent window and offset parameters
  int parentx, parenty, parentlenx, parentleny;
  int x, y, lenx, leny, parx, pary;
  // begyx gives coordinates (relative to stdscr) of upper-left corner.
  // maxyx gives *size*. max + beg - 1 is last addressable position.
  // paryx gives coordinates relative to parent window (-1 if not subwin)
  window_coordinates(w, &y, &x, &leny, &lenx);
  getbegyx(parent, parenty, parentx);
  getmaxyx(parent, parentleny, parentlenx);
  getparyx(w, pary, parx);
  int maxx, maxy;
  maxx = lenx + x - 1;
  maxy = leny + y - 1;
  if(y != parenty + pr->popts.toff){
    return -1;
  }
  assert(pary == -1);
  assert(parx == -1);
  if(x != parentx + pr->popts.loff){
    return -1;
  }
  if(leny != parentleny - (pr->popts.toff + pr->popts.boff)){
    return -1;
  }
  if(lenx != parentlenx - (pr->popts.loff + pr->popts.roff)){
    return -1;
  }
//  fprintf(stderr, "-----> VALIDATE PRTOTAL: %d/%d -> %d/%d (%d/%d)\n", x, y, maxx, maxy, lenx, leny);
  // Verify tablet placing and coverage of the space. Assuming we have
  // sufficient tablets, we ought cover [y, maxy], (y, maxy], or [y, maxy),
  // less reel borders. We ought otherwise cover [y, ...). Tablets ought be
  // spaced by the prescribed gap. Hidden tablets ought not have panels.
  int tstart = y - 1;
  int tend = maxy + 1;
  const int ltarg = x + !(pr->popts.bordermask & BORDERMASK_LEFT);
  const int rtarg = maxx - !(pr->popts.bordermask & BORDERMASK_RIGHT);
  if(pr->tablets){
    tablet* t = pr->tablets;
    PANEL* tp;
    WINDOW* tw;
    int tx, ty, lentx, lenty;
    bool allvisible = false;
    // FIXME can probably fold this mess into a single case
    do{ // work our way back from focus to the top
      tp = t->p;
      if(tp == NULL){ // FIXME verify that no later ones have a PANEL
        break;
      }
      tw = panel_window(tp);
      assert(tw);
      if(tw == NULL){ // panel should never lack a window
        return -1;
      }
      window_coordinates(tw, &ty, &tx, &lenty, &lentx);
      int maxtx = tx + lentx - 1;
      int maxty = ty + lenty - 1;
      if(tstart == y - 1){
        tstart = ty;
        tend = maxty;
//fprintf(stderr, "START %p TEND: %d TSTART: %d\n", t, tend, tstart);
      }else{
        if(tx != ltarg || maxtx != rtarg){
          assert(ltarg == tx);
          assert(rtarg == maxtx);
        }
        if(maxty < tstart){
          if(maxty != tstart - 2){
//fprintf(stderr, "BAD %p TSTART: %d TY: %d MAXTY: %d\n", t, tstart, ty, maxty);
            assert(maxty == tstart - 2);
            return -1;
          }
          tstart = ty;
//fprintf(stderr, "GOOD %p TSTART: %d TY: %d MAXTY: %d\n", t, tstart, ty, maxty);
        }else{
          if(ty > tend + 2){ // came back around, catch it below
            allvisible = true;
            break;
          }
          if(maxty >= tend){
//fprintf(stderr, "GOOD %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
            tend = maxty;
          }else{
//fprintf(stderr, "BADSKIP %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
          }
        }
      }
    }while((t = t->prev) != pr->tablets);
    if(t != pr->tablets){ // don't repeat if we covered all tablets
      // work our way down from the focus (not including the focus)
//fprintf(stderr, "EASTBOUND & DOWN TEND: %d T: %p TABS: %p\n", tend, t, pr->tablets);
      for(t = pr->tablets->next ; t != pr->tablets ; t = t->next){
        tp = t->p;
        if(tp == NULL){ // FIXME verify that no later ones have a PANEL
          break;
        }
        tw = panel_window(tp);
        if(tw == NULL){ // panel should never lack a window
          assert(tw);
          return -1;
        }
        window_coordinates(tw, &ty, &tx, &lenty, &lentx);
        int maxtx = tx + lentx - 1;
        int maxty = ty + lenty - 1;
        if(tx != ltarg || maxtx != rtarg){
          assert(ltarg == tx);
          assert(rtarg == maxtx);
        }
        if(maxty >= tend){
//fprintf(stderr, "GOOD %p TEND: %d TY: %d MAXTY: %d LENTY: %d\n", t, tend, ty, maxty, lenty);
          tend = maxty;
        }else{
//fprintf(stderr, "BADSKIP %p TEND: %d TY: %d MAXTY: %d LENTY: %d\n", t, tend, ty, maxty, lenty);
        }
      }
    }else{
      allvisible = true;
    }
    const int btarg = maxy - !(pr->popts.bordermask & BORDERMASK_BOTTOM);
//fprintf(stderr, "ALL: %d TSTART: %d TEND: %d BTARG: %d\n", allvisible, tstart, tend, btarg);
    // Coverage ought always begin at the top
    if(tstart != y + !(pr->popts.bordermask & BORDERMASK_TOP)){
      assert(tstart == y + !(pr->popts.bordermask & BORDERMASK_TOP));
      return -1;
    }
    if(!allvisible){ // filled up the space
      // tend must be within one gap of the bottom border
      if(tend < btarg - 2){
        assert(tend >= btarg - 2);
        return -1;
      }
    }
    if(tend > btarg){
      assert(tend <= btarg);
      return -1;
    }
  }
  return 0;
}

void* tablet_userptr(tablet* t){
  return t->curry;
}

const void* tablet_userptr_const(const tablet* t){
  return t->curry;
}
