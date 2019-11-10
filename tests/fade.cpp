#include "main.h"
#include <cstdlib>
#include <iostream>

void fade_setup(WINDOW* w) {
  wborder(w, 0, 0, 0, 0, 0, 0, 0, 0);
  wmove(w, 1, 1);
  const auto PERLINE = 16;
  for(int i = 0 ; i < COLORS ; i += PERLINE){
    wmove(w, i / PERLINE + 1, 1);
    for(int j = 0 ; j < PERLINE ; ++j){
      if(i + j >= COLORS){
        break;
      }
      wattrset(w, COLOR_PAIR(i + j));
      wprintw(w, "*");
      wattrset(w, A_DIM | COLOR_PAIR(i + j));
      wprintw(w, "*");
      wattrset(w, A_BOLD | COLOR_PAIR(i + j));
      wprintw(w, "*");
      if(j % PERLINE == PERLINE / 2){
        wprintw(w, " ");
      }
	  }
    wprintw(w, " (%3d)", i);
  }
  wrefresh(w);
}

TEST(OutcursesFade, FadeOut) {
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  ASSERT_NE(nullptr, init_outcurses(true));
  fade_setup(stdscr);
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 500000000, };
  nanosleep(&ts, NULL);
  ASSERT_EQ(0, fadeout(stdscr, 1000));
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesFade, FadeIn) {
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  ASSERT_NE(nullptr, init_outcurses(true));
  outcurses_rgb* palette = new outcurses_rgb[COLORS];
  retrieve_palette(COLORS, palette, nullptr, true);
  fade_setup(stdscr);
  ASSERT_EQ(0, fadein(stdscr, COLORS, palette, 1000));
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 500000000, };
  nanosleep(&ts, NULL);
  ASSERT_EQ(0, stop_outcurses(true));
  delete[] palette;
}
