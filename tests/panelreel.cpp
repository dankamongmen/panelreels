#include "main.h"
#include <iostream>

TEST(OutcursesPanelReel, InitLinear) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = { };
  ASSERT_EQ(0, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, InitCircular) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = {
    .circular = true,
  };
  ASSERT_EQ(0, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, destroy_panelreel(pr));
  ASSERT_EQ(0, stop_outcurses(true));
}
