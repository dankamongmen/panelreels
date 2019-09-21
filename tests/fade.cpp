#include "main.h"
#include <iostream>

void PrintColors() {
  mvwprintw(stdscr, 1, 1, "Color count: %d", COLORS);
  wrefresh(stdscr);
}

TEST(OutcursesFade, Fade) {
  ASSERT_EQ(0, Outcurses::init_outcurses(true));
  std::cerr << "Testing palette fade..." << std::endl;
  PrintColors();
  ASSERT_EQ(0, Outcurses::fade(stdscr, 2));
  // This should return OK, but fails in headless environments. Check
  // isendwin() afterwards as a proxy for this function, instead. FIXME
  Outcurses::stop_outcurses(true);
  ASSERT_EQ(true, isendwin());
}
