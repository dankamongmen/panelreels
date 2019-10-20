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

typedef struct panelreel {
  panelreel_options popts;
  tablet* tablets;         // doubly-linked list, circular for infinity scrolls
} panelreel;

panelreel* create_panelreel(const panelreel_options* popts){
  panelreel* pr;

  if(!popts->infinitescroll){
    if(popts->circular){
      return NULL; // can't set circular without infinitescroll
    }
  }
  if( (pr = malloc(sizeof(*pr))) ){
    pr->tablets = NULL;
    // FIXME verify margins work for window size
    memcpy(&pr->popts, popts, sizeof(*popts));
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
    // FIXME determine whether we're on the screen, and if so, draw us
  }
  return t;
}

int del_table(struct panelreel* pr, struct tablet* t){
  if(pr == NULL || t == NULL){
    return -1;
  }
  // FIXME
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
