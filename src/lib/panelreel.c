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
  --leny;
  --lenx;
  begx = 0;
  begy = 0;
  if(!cliphead){
    // lenx - begx + 1 is the number of columns we have, but drop 2 due to
    // corners. we thus want lenx - begx - 1 horizontal lines.
    if(!(nobordermask & BORDERMASK_TOP)){
      ret |= mvwadd_wch(w, begy, begx, &WACS_R_ULCORNER);
      ret |= whline_set(w, WACS_HLINE, lenx - begx - 1);
      ret |= mvwadd_wch(w, begy, begx + lenx, &WACS_R_URCORNER);
    }else{
      if(!(nobordermask & BORDERMASK_LEFT)){
        ret |= mvwadd_wch(w, begy, begx, &WACS_R_ULCORNER);
      }
      if(!(nobordermask & BORDERMASK_RIGHT)){
        ret |= mvwadd_wch(w, begy, lenx, &WACS_R_URCORNER);
      }
    }
  }
  int y;
  for(y = begy + 1 ; y < leny ; ++y){
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, y, begx, WACS_VLINE);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      ret |= mvwadd_wch(w, y, lenx, WACS_VLINE);
    }
  }
  if(!clipfoot){
    if(!(nobordermask & BORDERMASK_BOTTOM)){
      ret |= mvwadd_wch(w, leny, begx, &WACS_R_LLCORNER);
      ret |= whline_set(w, WACS_HLINE, lenx - begx - 1);
      mvwadd_wch(w, leny, begx + lenx, &WACS_R_LRCORNER); // always errors
    }else{
      if(!(nobordermask & BORDERMASK_LEFT)){
        ret |= mvwadd_wch(w, leny, begx, &WACS_R_LLCORNER);
      }
      if(!(nobordermask & BORDERMASK_RIGHT)){
        // mvwadd_wch returns error if we print to the lowermost+rightmost
        // character cell. maybe we can make this go away with scrolling controls
        // at setup? until then, don't check for error here FIXME.
        mvwadd_wch(w, leny, lenx, &WACS_R_LRCORNER);
      }
    }
  }
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
fprintf(stderr, "MAIN WINDOW: %d/%d -> %d/%d\n", *begy, *begx, *leny, *lenx);
  if(frontiery < 1){ // FIXME account for borders
fprintf(stderr, "FRONTIER DEAD UP TOP %d\n", frontiery);
    return -1;
  }
  if(frontiery >= *leny){
fprintf(stderr, "FRONTIER DEAD DOWN BELOW %d %d\n", frontiery, *leny);
    return -1;
  }
  --*lenx;
  --*leny;
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
  return 0;
}

// Draw the specified tablet, if possible. A direction less than 0 means we're
// laying out towards the top. Greater than zero means towards the bottom. 0
// means this is the focused tablet, always the first one to be drawn.
// frontiery is the line on which we're placing the tablet. For direction
// greater than or equal to 0, it's the top line of the tablet. For direction
// less than 0, it's the bottom line. Give the tablet all possible space to work
// with (i.e. up to the edge we're approaching, or the entire panel for the
// focused tablet). If the callback uses less space, shrink the panel back
// down before displaying it.
static int
panelreel_draw_tablet(const panelreel* pr, tablet* t, int frontiery,
                      int direction){
  int lenx, leny, begy, begx;
  WINDOW* w;
  PANEL* fp = t->p;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, direction)){
    if(fp){
      w = panel_window(fp);
      del_panel(fp);
      delwin(w);
      t->p = NULL;
      update_panels();
    }
    return 0;
  }
fprintf(stderr, "drawing FRONTIER %d %d/%d + %d/%d\n", frontiery, begy, begx, leny, lenx);
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
    t->update_pending = true;
  }else{
    w = panel_window(fp);
    int trueby = getbegy(w);
    int truey, truex;
    getmaxyx(w, truey, truex);
    int maxy = getmaxy(panel_window(pr->p)) + getbegy(panel_window(pr->p));
    if(truey > maxy - begy){
      if(wresize(w, maxy - begy, truex)){
        return -1;
      }
      update_panels();
      getmaxyx(w, truey, truex);
    }
    if(begy != trueby){
fprintf(stderr, "MOVING TO %d/%d\n", begy, begx);
      if(move_panel(fp, begy, begx)){
        assert(false);
        return -1;
      }
    }
  }
  if(t->update_pending){
    // discount for inhibited borders FIXME
fprintf(stderr, "resizing to %d/%d\n", leny, lenx + 1);
    wresize(w, leny, lenx + 1);
    int ll = t->cbfxn(fp, 1, 1, lenx - 1, leny, false, t->curry);
    t->update_pending = false;
    if(ll != leny - 2){
      wresize(w, ll + 2, lenx);
    }
    update_panels();
    bool cliphead = false; // direction < 0; // FIXME and...
    bool clipfoot = false; // direction > 0; // FIXME and...
    draw_borders(w, pr->popts.tabletmask,
                 direction == 0 ? pr->popts.focusedattr : pr->popts.tabletattr,
                 direction == 0 ? pr->popts.focusedpair : pr->popts.tabletpair,
                 cliphead, clipfoot);
  }
  return 0;
}

// Draw and place the focused tablet, which mustn't be NULL. direction is used
// the same way as it is in panelreel_arrange() below. frontiery is the line
// demarcating the frontier: if we're moving up, the bottom of this tablet
// should be at frontiery, and if we're moving down (or focused), the top of
// this tablet ought be there.
static int
panelreel_draw_focused(const panelreel* pr, tablet* focused){
  unsigned focusline = getbegy(panel_window(pr->p)); // FIXME
  if(!(pr->popts.bordermask & BORDERMASK_TOP)){
    ++focusline;
  }
  return panelreel_draw_tablet(pr, focused, focusline, 0);
}

// Arrange the panels, starting with the focused window, wherever it may be.
// Work in both directions until the screen is filled. If the screen is not
// filled, move everything towards the top. Supply -1 if we're moving up, 0 if
// this call is not in response to user movement, and 1 if we're moving down.
static int
panelreel_arrange(const panelreel* pr, int direction){
  int ret = 0;
  tablet* focused = pr->tablets;
  if(focused == NULL){
    return 0; // if none are focused, none exist
  }
  ret |= panelreel_draw_focused(pr, focused);
  int wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  int pbegy, pbegx, plenx, pleny; // panelreel window coordinates
  window_coordinates(panel_window(pr->p), &pbegy, &pbegx, &pleny, &plenx);
  int pmaxy = pbegy + pleny - 1;
  tablet* working = focused;
  window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
  int wmaxy = wbegy + wleny - 1;
  int frontiery = wmaxy + 2;
  // move down past the focused tablet, filling up the reel to the bottom
  while(frontiery < pmaxy){
    if((working = working->next) == focused){
      break;
    }
    ret |= panelreel_draw_tablet(pr, working, frontiery, 1);
    if(working->p){
      window_coordinates(panel_window(working->p), &wbegy, &wbegx, &wleny, &wlenx);
      wmaxy = wbegy + wleny;
      frontiery = wmaxy + 1;
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
  return 0;
}

int panelreel_redraw(const panelreel* pr){
  int ret = 0;
  if(draw_panelreel_borders(pr)){
    return -1; // enforces specified dimensional minima
  }
  ret |= panelreel_arrange(pr, 0);
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
    // inserted at the top, pushing down existing ones. The first one to be
    // added gets and keeps the focus, so eventually it hits the bottom, and
    // onscreen tablet growth starts pushing back against the top (at this
    // point, new tablets will be created off-screen until something changes).
    before = pr->tablets;
  }
  if( (t = malloc(sizeof(*t))) ){
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
    t->update_pending = false;
    t->p = NULL;
    ++pr->tabletcount;
fprintf(stderr, "ADDING TABLET %d\n", pr->tabletcount);
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
  t->update_pending = true;
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
