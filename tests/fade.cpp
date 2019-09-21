#include "main.h"
#include <iostream>

TEST(OutcursesFade, Fade){
  ASSERT_EQ(0, Outcurses::init_outcurses(true));
  std::cerr << "Testing palette fade..." << std::endl;
  ASSERT_EQ(0, Outcurses::fade(1));
  // This should return OK, but fails in headless environments. Check
  // isendwin() afterwards as a proxy for this function, instead. FIXME
  Outcurses::stop_outcurses(true);
  ASSERT_EQ(true, isendwin());
}
