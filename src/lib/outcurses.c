#include <outcurses.h>

int init_outcurses(bool initcurses){
	WINDOW* scr;

	if(initcurses){
		if((scr = initscr()) == NULL){
			fprintf(stderr, "Couldn't initialize ncurses\n");
			return -1;
		}
		if(start_color() != OK){
			fprintf(stderr, "Couldn't start color support\n");
			endwin();
			return -1;
		}
		if(assume_default_colors(-1, -1) != OK){
			fprintf(stderr, "Warning: couldn't assume default colors\n");
		}
    // FIXME need be checking errors on all these, and probably shouldn't be
    // embedding this functionality at all
		cbreak();
		noecho();
		nonl();
		intrflush(scr, FALSE);
		keypad(scr, TRUE);
		leaveok(scr, TRUE);
		curs_set(0);
	}else{
		if((scr = stdscr) == NULL){
			fprintf(stderr, "Couldn't get stdscr (was ncurses initialized?)\n");
			endwin();
			return -1;
		}
	}
	return 0;
}

int stop_outcurses(bool stopcurses){
	int ret = 0;
	if(stopcurses){
		if(endwin() != OK){
			fprintf(stderr, "Error during endwin()\n");
		}
	}
	return ret;
}
