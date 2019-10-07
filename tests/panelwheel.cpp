#include "main.h"
#include <iostream>

TEST(OutcursesPanelWheel, InitLinear) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelwheel p = { };
  ASSERT_EQ(0, init_outcurses(true));
  ASSERT_EQ(0, init_panelwheel(&p));
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelWheel, InitCircular) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelwheel p = {
    .circular = true,
  };
  ASSERT_EQ(0, init_outcurses(true));
  ASSERT_EQ(0, init_panelwheel(&p));
  ASSERT_EQ(0, stop_outcurses(true));
}
