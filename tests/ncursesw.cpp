#include "main.h"
#include <cstdlib>

// We want to ensure the wide-character version of Ncurses has been linked in,
// and that basic functionality works.
TEST(NCURSESW, AddWch){
  if(getenv("TERM") == nullptr){
	  GTEST_SKIP();
  }
  add_wch(WACS_BLOCK);
}
