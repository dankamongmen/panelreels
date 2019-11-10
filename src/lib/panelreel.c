#include <panel.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "outcurses.h"

// Tablets are the toplevel entitites within a panelreel. Each corresponds to
// a single, distinct PANEL.
typedef struct tablet {
  PANEL* p;
  void* curry;
  struct tablet* next;
  struct tablet* prev;
  tabletcb cbfxn;
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

// These ought be part of ncurses, probably

// Repeat the cchar_t ch n times at the current location in w.
static int
whwline(WINDOW *w, const cchar_t* ch, int n){
  while(n-- > 0){
    int ret;
    if((ret = wadd_wch(w, ch)) != OK){
      return ret;
    }
  }
  return OK;
}

static const cchar_t WACS_ULROUNDCORNER = { .attr = 0, .chars = L"╭", };
static const cchar_t WACS_URROUNDCORNER = { .attr = 0, .chars = L"╮", };
static const cchar_t WACS_LLROUNDCORNER = { .attr = 0, .chars = L"╰", };
static const cchar_t WACS_LRROUNDCORNER = { .attr = 0, .chars = L"╯", };

// bchrs: 6-element array of wide border characters + attributes
static int
draw_borders(WINDOW* w, unsigned nobordermask, int begx, int begy,
            int maxx, int maxy){
  int ret = OK;
  // maxx - begx + 1 is the number of columns we have, but drop 2 due to
  // corners. we thus want maxx - begx - 1 horizontal lines.
  if(!(nobordermask & BORDERMASK_TOP)){
    ret |= mvwadd_wch(w, begy, begx, &WACS_ULROUNDCORNER);
    ret |= whwline(w, WACS_HLINE, maxx - begx - 1);
    ret |= wadd_wch(w, &WACS_URROUNDCORNER);
  }else{
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, begy, begx, &WACS_ULROUNDCORNER);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      ret |= mvwadd_wch(w, begy, maxx, &WACS_URROUNDCORNER);
    }
  }
  int y;
  for(y = begy + 1 ; y < maxy ; ++y){
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, y, begx, WACS_VLINE);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      ret |= mvwadd_wch(w, y, maxx, WACS_VLINE);
    }
  }
  if(!(nobordermask & BORDERMASK_BOTTOM)){
    ret |= mvwadd_wch(w, maxy, begx, &WACS_LLROUNDCORNER);
    ret |= whwline(w, WACS_HLINE, maxx - begx - 1);
    wadd_wch(w, &WACS_LRROUNDCORNER);
  }else{
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, maxy, begx, &WACS_LLROUNDCORNER);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      // mvwadd_wch returns error if we print to the lowermost+rightmost
      // character cell. maybe we can make this go away with scrolling controls
      // at setup? until then, don't check for error here FIXME.
      mvwadd_wch(w, maxy, maxx, &WACS_LRROUNDCORNER);
    }
  }
  return ret;
}

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
  int pair = pr->popts.borderpair;
  wattr_set(w, pr->popts.borderattr, 0, &pair);
  return draw_borders(w, pr->popts.bordermask, begx, begy, maxx, maxy);
}

// Calculate the starting and ending columns available for occupation by
// tablets, relative to the panelreel's WINDOW.
static void
tablet_columns(const panelreel* pr, int* begx, int* begy, int* maxx, int* maxy){
  WINDOW* w = panel_window(pr->p);
  *begx = getbegx(w);
  *maxx = getmaxx(w);
  *begy = getbegy(w);
  *maxy = getmaxy(w);
  // account for the panelreel border
  if(!(pr->popts.bordermask & BORDERMASK_TOP)){
    ++*begy;
  }
  if(!(pr->popts.bordermask & BORDERMASK_BOTTOM)){
    --*maxy;
  }
  if(!(pr->popts.bordermask & BORDERMASK_LEFT)){
    ++*begx;
  }
  if(!(pr->popts.bordermask & BORDERMASK_RIGHT)){
    --*maxx;
  }
}

// Arrange the panels, starting with the focused window, wherever it may be.
// Work in both directions until the screen is filled. If the screen is not
// filled, move everything towards the top. Supply -1 if we're moving up, 0 if
// this call is not in response to user movement, and 1 if we're moving down.
static int
panelreel_arrange(const panelreel* pr, int direction){
  tablet* focused = pr->tablets;
  if(focused == NULL){
    return 0; // if none are focused, none exist
  }
  PANEL* fp = focused->p;
  if(fp == NULL){ // create a panel for the focused tablet
    int maxx, maxy, begy, begx, xlen, ylen;
    tablet_columns(pr, &begx, &begy, &maxx, &maxy);
    xlen = maxx - begx;
    ylen = maxy - begy;
    ylen = ylen > 3 ? 3 : ylen; // FIXME no, just for early testing
    WINDOW* w = derwin(panel_window(pr->p), ylen, xlen, begy, begx);
    if(w == NULL){
      return -1;
    }
    focused->p = new_panel(w);
    if((fp = focused->p) == NULL){
      delwin(w);
      return -1;
    }
    focused->cbfxn(fp, 0, 0, xlen, ylen, false, focused->curry);
    box(w, '|', '-'); // FIXME draw border
  }else{
    if(show_panel(fp)){
      return -1;
    }
  }
  // FIXME others!
  update_panels();
  doupdate();
  return 0;
}

int panelreel_redraw(const panelreel* pr){
  int ret = 0;
  if(draw_panelreel_borders(pr)){
    return -1; // enforces specified dimensional minima
  }
  ret |= panelreel_arrange(pr, 0);
  return ret;
}

panelreel* create_panelreel(WINDOW* w, const panelreel_options* popts,
                            int toff, int roff, int boff, int loff){
  panelreel* pr;

  if(w == NULL){
    return NULL;
  }
  if(!popts->infinitescroll){
    if(popts->circular){
      return NULL; // can't set circular without infinitescroll
    }
  }
  const unsigned fullmask = BORDERMASK_LEFT |
                            BORDERMASK_RIGHT |
                            BORDERMASK_TOP |
                            BORDERMASK_BOTTOM;
  if(popts->bordermask > fullmask){
    return NULL;
  }
  if(popts->tabletmask > fullmask){
    return NULL;
  }
  if( (pr = malloc(sizeof(*pr))) ){
    pr->tablets = NULL;
    pr->tabletcount = 0;
    memcpy(&pr->popts, popts, sizeof(*popts));
    int x, y;
    getmaxyx(w, y, x);
    --y;
    --x;
    int ylen, xlen;
    ylen = y - boff - toff - 1;
    if(ylen < 0){
      ylen = y - toff;
      if(ylen < 0){
        ylen = 0; // but this translates to a full-screen window...FIXME
      }
    }
    xlen = x - roff - loff - 1;
    if(xlen < 0){
      xlen = x - loff;
      if(xlen < 0){
        xlen = 0; // FIXME see above...
      }
    }
    WINDOW* pw = derwin(w, ylen, xlen, toff, loff);
    if((pr->p = new_panel(pw)) == NULL){
      free(pr);
      delwin(pw);
      return NULL;
    }
    if(panelreel_redraw(pr)){
      del_panel(pr->p);
      delwin(pw);
      free(pr);
      return NULL;
    }
  }
  return pr;
}

// Ought the specified tablet currently be (at least in part) visible? This is
// independent of whether it is currently visible, and derived instead from the
// prime source of truth regarding layout--the focused tablet, and its position.
// Returns true if the tablet is visible, setting *vline to the first row
// within the visible panelreel occupied by the tablet. Note that this might not
// be where the tablet's first line would be, if the tablet is only partially
// visible at the top of the reel. Tablets are never split across the top and
// bottom; they are zero or more contiguous lines in a reel.
/*static bool
tablet_visible_p(const panelreel* pr, tablet* t){
  // FIXME
  return true;
}*/

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
  // FIXME
  --pr->tabletcount;
  return 0;
}

int destroy_panelreel(panelreel* preel){
  int ret = 0;
  if(preel){
    tablet* t = preel->tablets;
    while(t){
      t->prev->next = NULL;
      tablet* tmp = t->next;
      free(t);
      t = tmp;
    }
    free(preel);
  }
  return ret;
}

int panelreel_tabletcount(const panelreel* preel){
  return preel->tabletcount;
}
