#include "main.h"
#include <iostream>

TEST(OutcursesFade, Fade) {
  ASSERT_EQ(0, Outcurses::init_outcurses(true));
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
	}
  }
  ASSERT_EQ(0, Outcurses::fade(stdscr, 2));
  // This should return OK, but fails in headless environments. Check
  // isendwin() afterwards as a proxy for this function, instead. FIXME
  ASSERT_EQ(0, Outcurses::stop_outcurses(true));
}
