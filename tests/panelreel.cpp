#include "main.h"
#include <iostream>

TEST(OutcursesPanelReel, InitLinear) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = { };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, InitLinearInfinite) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = {
    .infinitescroll = true,
  };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, InitCircular) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = {
    .infinitescroll = true,
    .circular = true,
  };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, destroy_panelreel(pr));
  ASSERT_EQ(0, stop_outcurses(true));
}

// circular is not allowed to be true when infinitescroll is false
TEST(OutcursesPanelReel, FiniteCircleRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = {
    .infinitescroll = false,
    .circular = true,
  };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, OnePanel) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = {
    .infinitescroll = false,
  };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(&p);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = add_tablet(pr, nullptr, nullptr, nullptr);
  ASSERT_NE(nullptr, t);
  // FIXME remove it
  ASSERT_EQ(0, stop_outcurses(true));
}
