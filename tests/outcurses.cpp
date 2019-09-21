#include "main.h"

// We want to ensure the wide-character version of Ncurses has been linked in,
// and that basic functionality works.
TEST(Outcurses, Init){
  ASSERT_EQ(0, Outcurses::init_outcurses(true));
  ASSERT_EQ(0, Outcurses::stop_outcurses(true));
}

// Ensure that colors are defined and work.
TEST(Outcurses, InitWithoutNcurses){
  Outcurses::init_outcurses(false);
  Outcurses::stop_outcurses(false);
}
