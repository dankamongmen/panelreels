#include "main.h"
#include <iostream>

TEST(OutcursesFade, Fade){
  ASSERT_EQ(0, Outcurses::init_outcurses(true));
  std::cerr << "Testing palette fade..." << std::endl;
  ASSERT_EQ(0, Outcurses::fade(1));
  ASSERT_EQ(0, Outcurses::stop_outcurses(true));
}
