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

int panelcb(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
            void* curry){
  EXPECT_NE(nullptr, p);
  EXPECT_LT(begx, maxx);
  EXPECT_LT(begy, maxy);
  EXPECT_EQ(nullptr, curry);
  EXPECT_FALSE(cliptop);
  return 0;
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

// Make a target window occupying all but a containing perimeter of the
// specified WINDOW (which will usually be stdscr).
PANEL* make_targwin(WINDOW* w) {
  cchar_t cc;
  int cpair = COLOR_GREEN;
  EXPECT_EQ(OK, setcchar(&cc, L"W", 0, 0, &cpair));
  int x, y, xx, yy;
  getbegyx(w, y, x);
  getmaxyx(w, yy, xx);
  yy -= 2;
  xx -= 2;
  ++x;
  ++y;
  WINDOW* ww = subwin(w, yy, xx, y, x);
  EXPECT_NE(nullptr, ww);
  PANEL* p = new_panel(ww);
  EXPECT_NE(nullptr, p);
  EXPECT_EQ(OK, wbkgrnd(ww, &cc));
  return p;
}

TEST(OutcursesPanelReel, InitWithinSubwin) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.loff = 1;
  p.roff = 1;
  p.toff = 1;
  p.boff = 1;
  p.infinitescroll = true;
  p.circular = true;
  ASSERT_NE(nullptr, init_outcurses(true));
  PANEL* base = make_targwin(stdscr);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = create_panelreel(basew, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, destroy_panelreel(pr));
  sleep(3); // time to inspect
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
  ASSERT_EQ(0, stop_outcurses(true));
}
