#include "main.h"
#include <iostream>

TEST(OutcursesFade, Fade) {
  ASSERT_EQ(0, init_outcurses(true));
  wmove(stdscr, 1, 1);
  const auto PERLINE = 16;
  for(int i = 0 ; i < COLORS ; i += PERLINE){
    wmove(stdscr, i / PERLINE + 1, 1);
    for(int j = 0 ; j < PERLINE ; ++j){
      if(i + j >= COLORS){
        break;
	  }
	  wattrset(stdscr, COLOR_PAIR(i + j));
	  wprintw(stdscr, "*");
	  wattrset(stdscr, A_DIM | COLOR_PAIR(i + j));
	  wprintw(stdscr, "*");
	  wattrset(stdscr, A_BOLD | COLOR_PAIR(i + j));
	  wprintw(stdscr, "*");
	}
  }
  ASSERT_EQ(0, fade(stdscr, 1500));
  // This should return OK, but fails in headless environments. Check
  // isendwin() afterwards as a proxy for this function, instead. FIXME
  ASSERT_EQ(0, stop_outcurses(true));
}
