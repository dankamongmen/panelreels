#include <panel.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "outcurses.h"

// Tablets are the toplevel entitites within a panelreel. Each corresponds to
// a single, distinct PANEL.
typedef struct tablet {
  PANEL* p;
  void* opaque;
  struct tablet* next;
  struct tablet* prev;
} tablet;

// The visible screen can be reconstructed from three things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (held by focusrow)
//  * the list of tablets (available from the focused tablet)
typedef struct panelreel {
  WINDOW* w;               // WINDOW this panelreel occupies
  panelreel_options popts;
  tablet* tablets;         // doubly-linked list, a circular one when infinity
    // scrolling is in effect. points at the focused tablet (when at least one
    // tablet exists, one must be focused), which might be anywhere on the
    // screen (but is guaranteed to be visible).
  int focusrow;            // row at which focused tablet starts
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
    ret |= wadd_wch(w, &WACS_LRROUNDCORNER);
  }else{
    if(!(nobordermask & BORDERMASK_LEFT)){
      ret |= mvwadd_wch(w, maxy, begx, &WACS_LLROUNDCORNER);
    }
    if(!(nobordermask & BORDERMASK_RIGHT)){
      ret |= mvwadd_wch(w, maxy, maxx, &WACS_LRROUNDCORNER);
    }
  }
  return ret;
}

static int
draw_panelreel_borders(const panelreel* pr, WINDOW* w){
  int begx, begy;
  int maxx, maxy;
  getbegyx(w, begy, begx);
  getmaxyx(w, maxy, maxx);
  assert(begy >= 0 && begx >= 0);
  assert(maxy >= 0 && maxx >= 0);
  begy += pr->popts.headerlines;
  begx += pr->popts.leftcolumns;
  --maxx; // last column we can safely write to
  --maxy; // last line we can safely write to
  maxy -= pr->popts.footerlines;
  maxx -= pr->popts.rightcolumns;
  if(begx + 1 >= maxx){
    return 0; // no room FIXME clear screen?
  }
  if(begy + 1 >= maxy){
    return 0; // no room FIXME clear screen?
  }
  return draw_borders(w, pr->popts.bordermask, begx, begy, maxx, maxy);
}

int panelreel_redraw(const panelreel* pr){
  return draw_panelreel_borders(pr, pr->w);
}

panelreel* create_panelreel(WINDOW* w, const panelreel_options* popts){
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
    pr->w = w;
    memcpy(&pr->popts, popts, sizeof(*popts));
    if(panelreel_redraw(pr)){
      free(pr);
      return NULL;
    }
  }
  return pr;
}

tablet* add_tablet(panelreel* pr, tablet* after, tablet *before, void* opaque){
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
    }
    t->opaque = opaque;
    ++pr->tabletcount;
    // FIXME determine whether we're on the screen, and if so, draw us
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
