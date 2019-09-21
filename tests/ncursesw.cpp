#include "main.h"

// We want to ensure the wide-character version of Ncurses has been linked in,
// and that basic functionality works.
TEST(NCURSESW, AddWch){
  add_wch(WACS_BLOCK);
}

// Ensure that colors are defined and work.
TEST(NCURSESW, Colors){
  ASSERT_EQ(true, has_colors());
}
