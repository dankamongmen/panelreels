#include "main.h"

// We want to ensure the wide-character version of Ncurses has been linked in,
// and that basic functionality works.
TEST(Outcurses, Init){
  ASSERT_EQ(0, init_outcurses(true));
  ASSERT_EQ(0, stop_outcurses(true));
}

// Ensure that colors are defined and work.
TEST(Outcurses, InitWithoutNcurses){
  ASSERT_EQ(0, init_outcurses(false));
  ASSERT_EQ(0, stop_outcurses(false));
}
