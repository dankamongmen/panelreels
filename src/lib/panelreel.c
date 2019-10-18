#include <stdlib.h>
#include <string.h>
#include "panelreel.h"

typedef struct panelreel {
  panelreel_options popts;
} panelreel;

panelreel* create_panelreel(const panelreel_options* popts){
  panelreel* pr;

  if( (pr = malloc(sizeof(*pr))) ){
    // FIXME verify margins work for window size
    memcpy(&pr->popts, popts, sizeof(*popts));
  }
  return 0;
}

int destroy_panelreel(panelreel* preel){
  int ret = 0;
  if(preel){
    free(preel);
  }
  return ret;
}
