#include <stdlib.h>
#include <outcurses.h>
#include <panelreel.h>

static int
panelreel_demo(struct panelreel* pr){
  struct tablet* t;
  add_tablet(pr, NULL, NULL, NULL);
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

int main(void){
  int ret = EXIT_FAILURE;

  if(init_outcurses(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
  print_intro(stdscr);
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
  };
  struct panelreel* pr = create_panelreel(&popts);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    goto done;
  }
  panelreel_demo(pr);
  if(destroy_panelreel(pr)){
    fprintf(stderr, "Error destroying panelreel\n");
    goto done;
  }
  ret = EXIT_SUCCESS;

done:
  if(stop_outcurses(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
	return ret;
}
