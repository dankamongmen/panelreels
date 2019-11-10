#include <stdlib.h>
#include <locale.h>
#include <outcurses.h>
#include "demo.h"

static void
tabletdraw(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
           void* curry){
  int cpair = random() % (COLORS - 16) + 16;
  int x, y;
  WINDOW* w = panel_window(p);
  for(y = begy ; y < maxy ; ++y){
    wmove(w, y, begx);
    cchar_t cch;
    setcchar(&cch, L"X", A_NORMAL, 0, &cpair);
    for(x = begx ; x < maxx ; ++x){
      wadd_wch(w, &cch);
    }
  }
}

static int
panelreel_demo(WINDOW* w, struct panelreel* pr){
  // Press a for a new panel above the current, c for a new one below the
  // current, and b for a new block at arbitrary placement. q quits.
  int pair = COLOR_CYAN;
  wattr_set(w, A_NORMAL, 0, &pair);
  int key;
  mvwprintw(w, 1, 1, "a, b, c create tablets, q quits.");
  clrtoeol();
  do{
    pair = COLOR_RED;
    wattr_set(w, A_NORMAL, 0, &pair);
    mvwprintw(w, 2, 2, "%d tablets", panelreel_tabletcount(pr));
    pair = COLOR_BLUE;
    wattr_set(w, A_NORMAL, 0, &pair);
    key = mvwgetch(w, 3, 2);
    clrtoeol();
    switch(key){
      case 'a': add_tablet(pr, NULL, NULL, tabletdraw, NULL); break;
      case 'b': add_tablet(pr, NULL, NULL, tabletdraw, NULL); break;
      case 'c': add_tablet(pr, NULL, NULL, tabletdraw, NULL); break;
      case KEY_DC: del_active_tablet(pr); break;
      case 'q': break;
      default: wprintw(w, "Unknown key: %c (%d)\n", key, key);
    }
  }while(key != 'q');
  return 0;
}

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
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
    .min_supported_cols = 8,
    .min_supported_rows = 5,
    .borderpair = (COLORS * (COLOR_MAGENTA + 1)) + 1,
    .borderattr = A_NORMAL,
  };
  struct panelreel* pr = create_panelreel(w, &popts, 4, 0, 0, 4);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return -1;
  }
  panelreel_demo(w, pr);
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
