#include <panel.h>
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
  atomic_bool update_pending;  // new data since the tablet was last drawn?
} tablet;

// The visible screen can be reconstructed from three things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (derived from focused window)
//  * the list of tablets (available from the focused tablet)
typedef struct panelreel {
  PANEL* p;                // PANEL this panelreel occupies, under tablets
  panelreel_options popts; // copied in create_panelreel()
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused), which might be anywhere on the screen (but is always visible).
  tablet* tablets;
  int tabletcount;         // could be derived, but we keep it o(1)
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
// tablet cannot be made visible as specified.
static int
tablet_columns(const panelreel* pr, int* begx, int* begy, int* lenx, int* leny,
               int frontiery, int direction){
  WINDOW* w = panel_window(pr->p);
  window_coordinates(w, begy, begx, leny, lenx);
  int maxy = *leny + *begy - 1;
  int begindraw = *begy + !(pr->popts.bordermask & BORDERMASK_TOP);
  int enddraw = maxy - !(pr->popts.bordermask & BORDERMASK_TOP);
  if(frontiery < begindraw){
    return -1;
  }
  if(frontiery >= enddraw){
// fprintf(stderr, "FRONTIER: %d ENDDRAW: %d\n", frontiery, enddraw);
    return -1;
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
  // this panelreel. this is the correct solution for the focused tablet.
  if(direction >= 0){
    *leny -= (frontiery - *begy);
    *begy = frontiery;
  }else if(direction < 0){
    *leny = frontiery - *begy;
  }
// fprintf(stderr, "tabletplacement: base %d/%d len %d/%d\n", *begx, *begy, *lenx, *leny);
  return 0;
}

// Draw the specified tablet, if possible. A direction less than 0 means we're
// laying out towards the top. Greater than zero means towards the bottom. 0
// means this is the focused tablet, always the first one to be drawn.
// frontiery is the line on which we're placing the tablet. For direction
// greater than or equal to 0, it's the top line of the tablet. For direction
// less than 0, it's the bottom line. Gives the tablet all possible space to
// work with (i.e. up to the edge we're approaching, or the entire panel for
// the focused tablet). If the callback uses less space, shrinks the panel back
// down before displaying it. Destroys any panel if it ought be hidden.
static int
panelreel_draw_tablet(const panelreel* pr, tablet* t, int frontiery,
                      int direction){
assert(direction >= 0); // FIXME don't yet support drawing up
  int lenx, leny, begy, begx;
  WINDOW* w;
  PANEL* fp = t->p;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, direction)){
// fprintf(stderr, "FRONTIER DONE!!!!!!\n");
    if(fp){
// fprintf(stderr, "HIDING %p at frontier %d (dir %d) with %d\n", t, frontiery, direction, leny);
      w = panel_window(fp);
      del_panel(fp);
      delwin(w);
      t->p = NULL;
      update_panels();
    }
    return 0;
  }
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
    atomic_store(&t->update_pending, true);
  }else{
    w = panel_window(fp);
    int trueby = getbegy(w);
    int truey, truex;
    getmaxyx(w, truey, truex);
// fprintf(stderr, "TRUEY: %d BEGY: %d LENY: %d\n", truey, begy, leny);
    if(truey > leny){
      if(wresize(w, leny, truex)){
        return -1;
      }
      update_panels();
      getmaxyx(w, truey, truex);
    }
    if(begy != trueby){
      if(move_panel(fp, begy, begx)){
        assert(false);
        return -1;
      }
    }
  }
  if(t->update_pending){
    wresize(w, leny, lenx);
    bool cliphead = false;
    bool clipfoot = false;
    // We pass the coordinates in which the callback may freely write. That's
    // the full width (minus tablet borders), and the full range of open space
    // in the direction we're moving.
    // Coordinates within the tablet window where the callback function may
    // freely write. This is everywhere in the tablet save tabletborders.
    int cby = 0, cbx = 0, cbmaxy = leny, cbmaxx = lenx;
    // If we're drawing up, we'll always have a bottom border unless it's masked
    if(direction < 0 && !(pr->popts.tabletmask & BORDERMASK_BOTTOM)){
      --cbmaxy;
    }
    // If we're drawing down, we'll always have a top border unless it's masked
    if(direction >= 0 && !(pr->popts.tabletmask & BORDERMASK_TOP)){
      ++cby;
      --cbmaxy;
    }
    cbmaxx -= !(pr->popts.tabletmask & BORDERMASK_RIGHT);
    cbmaxx -= !(pr->popts.tabletmask & BORDERMASK_LEFT);
    cbx += !(pr->popts.tabletmask & BORDERMASK_LEFT);
    bool cbdir = direction < 0 ? true : false;
// fprintf(stderr, "calling! lenx/leny: %d/%d cbx/cby: %d/%d cbmaxx/cbmaxy: %d/%d dir: %d\n",
//    lenx, leny, cbx, cby, cbmaxx, cbmaxy, direction);
    // FIXME if cbmaxy < cby don't call, but that shouldn't happen?
    int ll = t->cbfxn(fp, cbx, cby, cbmaxx, cbmaxy, cbdir, t->curry);
    atomic_store(&t->update_pending, false);
    if(ll != leny){
      if(ll == cbmaxy){
// fprintf(stderr, "RESIZING (-1) from %d to %d\n", leny, ll + 1);
        wresize(w, ll + 1, lenx);
        if(direction < 0){
          cliphead = true;
        }else{
          clipfoot = true;
        }
      }else{
// fprintf(stderr, "RESIZING (-2) from %d to %d\n", leny, ll + 2);
//         wresize(w, ll + 2, lenx);
      }
    }
    update_panels();
    draw_borders(w, pr->popts.tabletmask,
                 direction == 0 ? pr->popts.focusedattr : pr->popts.tabletattr,
                 direction == 0 ? pr->popts.focusedpair : pr->popts.tabletpair,
                 cliphead, clipfoot);
  }
  return 0;
}

// Arrange the panels, starting with the focused window, wherever it may be.
// Work in both directions until the screen is filled. If the screen is not
// filled, move everything towards the top.
static int
panelreel_arrange(const panelreel* pr){
  int ret = 0;
  tablet* focused = pr->tablets;
  if(focused == NULL){
    return 0; // if none are focused, none exist
  }
  int pbegy, pbegx, plenx, pleny; // panelreel window coordinates
  window_coordinates(panel_window(pr->p), &pbegy, &pbegx, &pleny, &plenx);
  // FIXME preserve focusline across calls. for now, place focus at top
  const unsigned focusline = pbegy + !(pr->popts.bordermask & BORDERMASK_TOP);
  ret |= panelreel_draw_tablet(pr, focused, focusline, 0);
  int wmaxy, wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  tablet* working = focused;
  int frontiery;
  // move down past the focused tablet, filling up the reel to the bottom
  while(working->next != focused){
    window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
    wmaxy = wbegy + wleny - 1;
    frontiery = wmaxy + 2;
    working = working->next;
    ret |= panelreel_draw_tablet(pr, working, frontiery, 1);
    if(working->p == NULL){ // FIXME might be more to hide
      break;
    }
  }
  // FIXME keep going forward, hiding those no longer visible
  // move up above the focused tablet, filling up the reel to the top
  working = focused;
  window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
  frontiery = wbegy - 1;
  while(frontiery > pbegy + 1){
    if((working = working->prev) == focused){
      break;
    }
    ret |= panelreel_draw_tablet(pr, working, frontiery, -1);
    if(working->p){
      window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
      frontiery = wbegy - 2;
    }
  }
  // FIXME keep going backwards, hiding those no longer visible
// fprintf(stderr, "DONE ARRANGING\n");
  return 0;
}

int panelreel_redraw(const panelreel* pr){
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

panelreel* create_panelreel(WINDOW* w, const panelreel_options* popts){
  panelreel* pr;

  if(!validate_panelreel_opts(w, popts)){
    return NULL;
  }
  if((pr = malloc(sizeof(*pr))) == NULL){
    return NULL;
  }
  pr->tablets = NULL;
  pr->tabletcount = 0;
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

tablet* add_tablet(panelreel* pr, tablet* after, tablet *before,
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
  if( (t = malloc(sizeof(*t))) ){
// fprintf(stderr, "--------->NEW TABLET %p\n", t);
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
    atomic_init(&t->update_pending, false);
    t->p = NULL;
    ++pr->tabletcount;
    if(panelreel_redraw(pr)){
      return NULL; // FIXME
    }
  }
  return t;
}

int del_active_tablet(struct panelreel* pr){
  return del_tablet(pr, pr->tablets);
}

int del_tablet(struct panelreel* pr, struct tablet* t){
  if(pr == NULL || t == NULL){
    return -1;
  }
  if(t->prev){
    t->prev->next = t->next;
  }
  if(t->next){
    t->next->prev = t->prev;
  }
  if(t->p){
    WINDOW* w = panel_window(t->p);
    del_panel(t->p);
    delwin(w);
  }
  if(pr->tablets == t){
    if((pr->tablets = t->next) == t){
      pr->tablets = NULL;
    }
  }
  free(t);
  --pr->tabletcount;
  update_panels();
  panelreel_redraw(pr);
  return 0;
}

int destroy_panelreel(panelreel* preel){
  int ret = 0;
  if(preel){
    tablet* t = preel->tablets;
    while(t){
      t->prev->next = NULL;
      tablet* tmp = t->next;
      del_tablet(preel, t);
      t = tmp;
    }
    free(preel);
  }
  return ret;
}

int panelreel_tabletcount(const panelreel* preel){
  return preel->tabletcount;
}

int tablet_update(panelreel* pr, tablet* t){
  (void)pr;
  atomic_store(&t->update_pending, true);
  return 0;
}

// Move to some position relative to the current position
static int
move_reel_panel(PANEL* p, int deltax, int deltay){
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

int panelreel_move(panelreel* preel, int x, int y){
  WINDOW* w = panel_window(preel->p);
  int oldx, oldy;
  getbegyx(w, oldy, oldx);
  const int deltax = x - oldx;
  const int deltay = y - oldy;
  if(move_reel_panel(preel->p, deltax, deltay)){
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
      move_reel_panel(t->p, deltax, deltay);
    }while((t = t->prev) != preel->tablets);
    if(t != preel->tablets){ // don't repeat if we covered all tablets
      for(t = preel->tablets->next ; t != preel->tablets ; t = t->next){
        if(t->p == NULL){
          break;
        }
        move_reel_panel(t->p, deltax, deltay);
      }
    }
  }
  update_panels();
  panelreel_redraw(preel);
  return 0;
}

int panelreel_next(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->next;
  }
  return panelreel_redraw(pr);
}

int panelreel_prev(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->prev;
  }
  return panelreel_redraw(pr);
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
  /* we're not a subwin
  assert(pary == pr->popts.toff);
  if(pary != pr->popts.toff){
    return -1;
  }
  assert(parx == pr->popts.loff);
  if(parx != pr->popts.loff){
    return -1;
  }*/
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
// fprintf(stderr, "START %p TEND: %d TSTART: %d\n", t, tend, tstart);
      }else{
        if(tx != ltarg || maxtx != rtarg){
          assert(ltarg == tx);
          assert(rtarg == maxtx);
        }
        if(maxty < tstart){
          if(maxty != tstart - 2){
// fprintf(stderr, "BAD %p TSTART: %d TY: %d MAXTY: %d\n", t, tstart, ty, maxty);
            assert(maxty == tstart - 2);
            return -1;
          }
          tstart = ty;
// fprintf(stderr, "GOOD %p TSTART: %d TY: %d MAXTY: %d\n", t, tstart, ty, maxty);
        }else{
          if(ty > tend + 2){ // came back around, catch it below
            allvisible = true;
            break;
          }
          if(ty != tend + 2){
// fprintf(stderr, "BAD %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
            assert(ty == tend + 2);
            return -1;
          }
// fprintf(stderr, "GOOD %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
          tend = maxty;
        }
      }
    }while((t = t->prev) != pr->tablets);
    if(t != pr->tablets){ // don't repeat if we covered all tablets
      // work our way down from the focus (not including the focus)
// fprintf(stderr, "EASTBOUND &  DOWN T: %p TABS: %p\n", t, pr->tablets);
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
        if(ty != tend + 2){
// fprintf(stderr, "BAD %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
          assert(ty == tend + 2);
          return -1;
        }
// fprintf(stderr, "GOOD %p TEND: %d TY: %d MAXTY: %d\n", t, tend, ty, maxty);
        tend = maxty;
      }
    }else{
      allvisible = true;
    }
    const int btarg = maxy - !(pr->popts.bordermask & BORDERMASK_BOTTOM);
// fprintf(stderr, "ALL: %d TSTART: %d TEND: %d BTARG: %d\n", allvisible, tstart, tend, btarg);
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
