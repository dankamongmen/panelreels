#include "main.h"
#include <iostream>

TEST(OutcursesPanelReel, InitLinear) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = { };
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, InitLinearInfinite) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = true;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, InitCircular) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = true;
  p.circular = true;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, destroy_panelreel(pr));
  ASSERT_EQ(0, stop_outcurses(true));
}

// circular is not allowed to be true when infinitescroll is false
TEST(OutcursesPanelReel, FiniteCircleRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  p.circular = true;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

void panelcb(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
             void* curry){
}

TEST(OutcursesPanelReel, OnePanel) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = add_tablet(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  // FIXME remove it
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, NoBorder) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.bordermask = BORDERMASK_LEFT | BORDERMASK_RIGHT |
                  BORDERMASK_TOP | BORDERMASK_BOTTOM;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, BadBorderBitsRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.bordermask = BORDERMASK_LEFT * 2;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}

TEST(OutcursesPanelReel, NoTabletBorder) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.tabletmask = BORDERMASK_LEFT | BORDERMASK_RIGHT |
                  BORDERMASK_TOP | BORDERMASK_BOTTOM;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}
TEST(OutcursesPanelReel, BadTabletBorderBitsRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.tabletmask = BORDERMASK_LEFT * 2;
  ASSERT_NE(nullptr, init_outcurses(true));
  struct panelreel* pr = create_panelreel(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, stop_outcurses(true));
}
