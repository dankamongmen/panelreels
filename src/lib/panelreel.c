#include <panel.h>
#include <stdlib.h>
#include <string.h>
#include "panelreel.h"

// Tablets are the toplevel entitites within a panelreel. Each corresponds to
// a single, distinct PANEL.
typedef struct tablet {
  PANEL* p;
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
