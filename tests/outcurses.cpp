#include "main.h"
#include <cstdlib>

// We want to ensure the wide-character version of Ncurses has been linked in,
// and that basic functionality works.
TEST(Outcurses, Init){
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  ASSERT_NE(nullptr, outcurses_init(true));
  ASSERT_EQ(0, outcurses_stop(true));
}

// Ensure that colors are defined and work.
TEST(Outcurses, InitWithoutNcurses){
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  ASSERT_NE(nullptr, initscr());
  ASSERT_NE(nullptr, outcurses_init(false));
  ASSERT_EQ(0, outcurses_stop(false));
}
