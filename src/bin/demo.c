#include <stdlib.h>
#include <outcurses.h>
#include <panelreel.h>

int main(void){
  int ret = EXIT_FAILURE;

  if(init_outcurses(true)){
    fprintf(stderr, "Error initializing outcurses\n");
    return EXIT_FAILURE;
  }
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
  };
  struct panelreel* pr = create_panelreel(&popts);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    goto done;
  }
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
