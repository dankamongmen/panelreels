#include <stdlib.h>
#include <outcurses.h>

static int
panelreel_demo(struct panelreel* pr){
  struct tablet* t;
  t = add_tablet(pr, NULL, NULL, NULL);
  return 0;
}

static void
print_intro(WINDOW *w){
  int key;

  mvwprintw(w, 1, 1, "About to run the Outcurses demo. Press any key to continue...\n");
  do{
    key = getch();
    fprintf(stderr, "Keypress: %d\n", key);
  }while(key == ERR);
}

static int
demo(WINDOW* w){
  print_intro(w);
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
  };
  struct panelreel* pr = create_panelreel(&popts);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return -1;
  }
  panelreel_demo(pr);
  fade(w, 1000);
  if(destroy_panelreel(pr)){
    fprintf(stderr, "Error destroying panelreel\n");
    return -1;
  }
  return 0;
}

int main(void){
  int ret = EXIT_FAILURE;
  WINDOW* w;

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
