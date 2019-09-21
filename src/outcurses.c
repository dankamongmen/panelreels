#include <outcurses.h>

int init_outcurses(bool initcurses){
	WINDOW* scr;

	if(initcurses){
		if((scr = initscr()) == NULL){
			fprintf(stderr, "Couldn't initialize ncurses\n");
			return -1;
		}
		cbreak();
		noecho();
		nonl();
		intrflush(stdscr, FALSE);
		keypad(stdscr, TRUE);
	}else{
		if((scr = stdscr) == NULL){
			fprintf(stderr, "Couldn't get stdscr (was ncurses initialized?)\n");
			return -1;
		}
	}
	return 0;
}

int stop_outcurses(bool stopcurses){
	int ret = 0;
	if(stopcurses){
		if(endwin()){
			ret = -1;
			fprintf(stderr, "Error during endwin()\n");
		}
	}
	return ret;
}
