#include "main.h"
#include <cstdlib>
#include <iostream>

TEST(OutcursesFade, Fade) {
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  ASSERT_NE(nullptr, init_outcurses(true));
  wborder(stdscr, 0, 0, 0, 0, 0, 0, 0, 0);
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
      if(j % PERLINE == PERLINE / 2){
        wprintw(stdscr, " ");
      }
	  }
    wprintw(stdscr, " (%3d)", i);
  }
  wrefresh(stdscr);
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 500000000, };
  nanosleep(&ts, NULL);
  ASSERT_EQ(0, fade(stdscr, 1500));
  ASSERT_EQ(0, stop_outcurses(true));
}
