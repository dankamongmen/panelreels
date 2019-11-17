#include "main.h"
#include <cstdlib>
#include <iostream>

void fade_setup(WINDOW* w) {
  EXPECT_EQ(OK, wborder(w, 0, 0, 0, 0, 0, 0, 0, 0));
  EXPECT_EQ(OK, wmove(w, 1, 1));
  const auto PERLINE = 16;
  for(int i = 0 ; i < COLORS ; i += PERLINE){
    EXPECT_EQ(OK, wmove(w, i / PERLINE + 1, 1));
    for(int j = 0 ; j < PERLINE ; ++j){
      if(i + j >= COLORS){
        break;
      }
      int cpair = i + j;
      EXPECT_EQ(OK, wattr_set(w, A_NORMAL, 0, &cpair));
      EXPECT_EQ(OK, wprintw(w, "*"));
      EXPECT_EQ(OK, wattr_set(w, A_DIM, 0, &cpair));
      EXPECT_EQ(OK, wprintw(w, "*"));
      EXPECT_EQ(OK, wattr_set(w, A_BOLD, 0, &cpair));
      EXPECT_EQ(OK, wprintw(w, "*"));
      if(j % PERLINE == PERLINE / 2){
        EXPECT_EQ(OK, wprintw(w, " "));
      }
	  }
    EXPECT_EQ(OK, wprintw(w, " (%3d)", i));
  }
  EXPECT_EQ(OK, wrefresh(w));
}

TEST(OutcursesFade, FadeOut) {
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  ASSERT_NE(nullptr, outcurses_init(true));
  fade_setup(stdscr);
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 500000000, };
  nanosleep(&ts, NULL);
  ASSERT_EQ(0, fadeout(stdscr, 1000));
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST(OutcursesFade, FadeIn) {
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  ASSERT_NE(nullptr, outcurses_init(true));
  outcurses_rgb* palette = new outcurses_rgb[COLORS];
  retrieve_palette(COLORS, palette, nullptr, true);
  fade_setup(stdscr);
  ASSERT_EQ(0, fadein(stdscr, COLORS, palette, 1000));
  struct timespec ts = {.tv_sec = 0, .tv_nsec = 500000000, };
  nanosleep(&ts, NULL);
  ASSERT_EQ(0, outcurses_stop(true));
  delete[] palette;
}
