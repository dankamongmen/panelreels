#include <panel.h>
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

#define WACS_L_LLCORNER L"╰"
#define WACS_L_LRCORNER L"╯"
#define WACS_L_ULCORNER L"╭"
#define WACS_L_URCORNER L"╮"
#define WACS_L_HLINE L"─"
#define WACS_L_VLINE L"│"

// Nicest border corners, but they don't line up with all fonts :/
static const cchar_t lightboxchars[] = {
  { .attr = 0, .chars = WACS_L_ULCORNER, },
  { .attr = 0, .chars = WACS_L_URCORNER, },
  { .attr = 0, .chars = WACS_L_LLCORNER, },
  { .attr = 0, .chars = WACS_L_LRCORNER, },
  { .attr = 0, .chars = WACS_L_VLINE, },
  { .attr = 0, .chars = WACS_L_HLINE, },
};

// bchrs: 6-element array of wide border characters + attributes
static int
draw_borders(WINDOW* w, unsigned nobordermask, int begx, int begy,
            int maxx, int maxy, const cchar_t* bchrs){
  int ret = OK;
  // FIXME might still need a ULCORNER/URCORNER...
  if(!(nobordermask & BORDERMASK_TOP)){
    ret |= mvwadd_wch(w, begx, begy, &bchrs[0]);
    ret |= whwline(w, &bchrs[5], maxx - 3 - begx);
    ret |= wadd_wch(w, &bchrs[1]);
  }
  if(!(nobordermask & BORDERMASK_BOTTOM)){
    ret |= mvwadd_wch(w, begx, maxy, &bchrs[2]);
    ret |= whwline(w, &bchrs[5], maxx - 3 - begx);
    ret |= wadd_wch(w, &bchrs[3]);
  }
  return ret;
}

static int
draw_panelreel_borders(const panelreel* pr, WINDOW* w){
  int begx, begy;
  int maxx, maxy;
  getbegyx(w, begy, begx);
  getmaxyx(w, maxy, maxx);
  begx += pr->popts.headerlines;
  begy += pr->popts.leftcolumns;
  maxx -= pr->popts.footerlines;
  maxy -= pr->popts.rightcolumns;
  if(begx + 1 >= maxx){
    return 0; // no room FIXME clear screen?
  }
  if(begy + 1 >= maxy){
    return 0; // no room FIXME clear screen?
  }
  return draw_borders(w, pr->popts.bordermask, begx, begy, maxx, maxy,
                      lightboxchars);
}

panelreel* create_panelreel(const panelreel_options* popts){
  panelreel* pr;

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
    draw_panelreel_borders(pr, pr->popts.w);
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

int del_table(struct panelreel* pr, struct tablet* t){
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
