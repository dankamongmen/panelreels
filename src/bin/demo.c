#include <stdlib.h>
#include <getopt.h>
#include <locale.h>
#include <outcurses.h>
#include "demo.h"

static void
print_intro(WINDOW *w){
  int key, pair;

  pair = COLOR_GREEN;
  wattr_set(w, A_STANDOUT, 0, &pair);
  mvwprintw(w, 1, 1, "Press any key to run the Outcurses %s demo...\n",
            outcurses_version());
  pair = COLOR_GREEN;
  wattr_set(w, A_NORMAL, 0, &pair);
  mvwprintw(w, 2, 2, "Colors: %d Pairs: %d Maxx: %d Maxy: %d\n",
            COLORS, COLOR_PAIRS, getmaxx(w), getmaxy(w));
  do{
    key = wgetch(w);
  }while(key == ERR);
}

static void
usage(const char* basename, int status){
  FILE* f = status == EXIT_SUCCESS ? stdout : stderr;
  fprintf(f, "usage: %s [ -h | -w | -p ]\n", basename);
  fprintf(f, " -p: only panelreel demo\n");
  fprintf(f, " -w: only widechar demo\n");
  fprintf(f, " -h: this message\n");
  exit(status);
}

int main(int argc, char** argv){
  bool only_panelreel = false;
  bool only_widechar = false;
  WINDOW* w;

  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Coudln't set locale based on user preferences\n");
    return EXIT_FAILURE;
  }
  int c;
  while((c = getopt(argc, argv, "hpw")) != EOF){
    switch(c){
      case 'p':
        if(only_widechar){
          usage(*argv, EXIT_FAILURE);
        }
        only_panelreel = true;
        break;
      case 'w':
        if(only_panelreel){
          usage(*argv, EXIT_FAILURE);
        }
        only_widechar = true;
        break;
      case 'h':
        usage(*argv, EXIT_SUCCESS);
        break;
      default:
        usage(*argv, EXIT_FAILURE);
        break;
    }
  }
  if((w = outcurses_init(true)) == NULL){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
  int ret = EXIT_SUCCESS;
  if(!only_panelreel && !only_widechar){
    print_intro(w);
  }
  if(!only_panelreel){
    ret |= widecolor_demo(w);
  }
  if(!only_widechar){
    ret |= panelreel_demo(w);
  }
  if(outcurses_stop(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
	return ret;
}
