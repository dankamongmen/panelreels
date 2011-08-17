#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ncursesw/ncurses.h>

static short r,g,b;

#define OUTCOLOR COLOR_YELLOW

static void
output(WINDOW *w){
	assert(wattron(w,A_BOLD | COLOR_PAIR(1)) == OK);
	assert(mvwprintw(w,1,1,"Hello world\n") == OK);
	assert(wattroff(w,A_BOLD) == OK);
	assert(mvwprintw(w,2,1,"Hello world\n") == OK);
	assert(mvwprintw(w,3,1,"r/g/b: %hd %hd %hd\n",r,g,b) == OK);
	wrefresh(w);
	doupdate();
}

int main(void){
	int z,canchange;
	WINDOW *scr;

	assert((scr = initscr()));
	assert(start_color() == OK);
	scrollok(scr,TRUE);
	canchange = can_change_color();
	// Supported TERMs: "linux", "xterm-256color"
	assert(canchange);
	assert(init_pair(1,OUTCOLOR,COLOR_BLACK) == OK);
	assert(color_content(OUTCOLOR,&r,&g,&b) == OK);
	while(r || g || b){
		output(scr);
		usleep(20000);
		if(r) --r;
		if(g) --g;
		if(b) --b;
		init_color(OUTCOLOR,r,g,1000);
	}
	assert(endwin() == OK);
	return EXIT_SUCCESS;
}
