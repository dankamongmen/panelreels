#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ncursesw/ncurses.h>

static short r,g,b;

#define OUTCOLOR COLOR_YELLOW
#define OUTCOLOR2 COLOR_BLUE

static void
output(WINDOW *w){
	if(w != curscr){
		assert(wattrset(w,A_DIM | COLOR_PAIR(1)) == OK);
		box(w,0,0);
		assert(mvwprintw(w,1,1,"Hello world\n") == OK);
		assert(wattrset(w,COLOR_PAIR(1)) == OK);
		assert(mvwprintw(w,2,1,"Hello world\n") == OK);
		assert(mvwprintw(w,3,1,"r/g/b: %hd %hd %hd\n",r,g,random()) == OK);
	}
	wrefresh(w);
}

int main(void){
	int z,canchange;
	WINDOW *scr;

	assert((scr = initscr()));
	assert(noecho() == OK);
	assert(start_color() == OK);
	scrollok(scr,TRUE);
	leaveok(scr,TRUE);
	curs_set(0);
	canchange = can_change_color();
	use_default_colors();
	// Supported TERMs: "linux", "xterm-256color"
	assert(canchange);
	assert(init_pair(1,OUTCOLOR,-1) == OK);
	assert(color_content(OUTCOLOR,&r,&g,&b) == OK);
	if(r) r = 1000;
	if(g) g = 1000;
	if(b) b = 1000;
	output(scr);
	while(r || g || b){
		output(curscr);
		// Don't want a period here above the screen's refresh rate, or
		// else we get crappy partial updates.
		usleep(30000);
		if((r -= 10) < 0){
			r = 0;
		}
		if((g -= 10) < 0){
			g = 0;
		}
		if((b -= 10) < 0){
			b = 0;
		}
		init_color(OUTCOLOR,r,g,b);
		assert(init_pair(1,OUTCOLOR,-1) == OK);
	}
	assert(endwin() == OK);
	return EXIT_SUCCESS;
}
