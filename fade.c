#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ncursesw/ncurses.h>

static void
output(WINDOW *w){
	assert(wattron(w,A_BOLD | COLOR_PAIR(1)) == OK);
	assert(mvwprintw(w,1,1,"Hello world\n") == OK);
	assert(wattroff(w,A_BOLD) == OK);
	assert(mvwprintw(w,2,1,"Hello world\n") == OK);
	wrefresh(w);
	doupdate();
}

int main(void){
	int z,canchange;
	WINDOW *scr;
	short r,g,b;

	assert((scr = initscr()));
	assert(start_color() == OK);
	scrollok(scr,TRUE);
	canchange = can_change_color();
	// Supported TERMs: "linux", "xterm-256color"
	assert(canchange);
	assert(init_pair(1,COLOR_YELLOW,COLOR_BLACK) == OK);
	output(scr);
	sleep(2);
	assert(endwin() == OK);
	return EXIT_SUCCESS;
}
