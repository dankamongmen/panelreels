#include <stdlib.h>
#include <locale.h>
#include <outcurses.h>
#include "demo.h"

static void
print_intro(WINDOW *w){
  int key, pair;

  pair = COLOR_GREEN;
  wattr_set(w, A_NORMAL, 0, &pair);
  mvwprintw(w, 1, 1, "About to run the Outcurses demo. Press any key to continue...\n");
  do{
    key = wgetch(w);
  }while(key == ERR);
}

static int
demo(WINDOW* w){
  print_intro(w);
  widecolor_demo(w);
  struct panelreel* pr;
  if((pr = panelreel_demo(w)) == NULL){
    return -1;
  }
  fadeout(w, FADE_MILLISECONDS);
  if(destroy_panelreel(pr)){
    fprintf(stderr, "Error destroying panelreel\n");
    return -1;
  }
  return 0;
}

int main(void){
  int ret = EXIT_FAILURE;
  WINDOW* w;

  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Coudln't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  if((w = init_outcurses(true)) == NULL){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
  if(demo(w) == 0){
    ret = EXIT_SUCCESS;
  }
  if(stop_outcurses(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
	return ret;
}
