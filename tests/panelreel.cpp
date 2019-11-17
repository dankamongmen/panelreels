#include "main.h"
#include <iostream>

class PanelReelTest : public :: testing::Test {
  void TearDown() override {
    endwin();
  }
};

TEST_F(PanelReelTest, InitLinear) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p = { };
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, InitLinearInfinite) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = true;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, InitCircular) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = true;
  p.circular = true;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, panelreel_destroy(pr));
  ASSERT_EQ(0, outcurses_stop(true));
}

// circular is not allowed to be true when infinitescroll is false
TEST_F(PanelReelTest, FiniteCircleRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  p.circular = true;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

// We ought be able to invoke panelreel_next() and panelreel_prev() safely,
// even if there are no tablets.
TEST_F(PanelReelTest, MovementWithoutTablets) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_next(pr));
  EXPECT_EQ(0, panelreel_prev(pr));
  ASSERT_EQ(0, outcurses_stop(true));
}

int panelcb(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
            void* curry){
  EXPECT_NE(nullptr, p);
  EXPECT_LT(begx, maxx);
  EXPECT_LT(begy, maxy);
  EXPECT_EQ(nullptr, curry);
  EXPECT_FALSE(cliptop);
  // FIXME verify geometry is as expected
  return 0;
}

TEST_F(PanelReelTest, OneTablet) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  EXPECT_EQ(0, panelreel_del(pr, t));
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, DeleteActiveTablet) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.infinitescroll = false;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  EXPECT_EQ(0, panelreel_del_active(pr));
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, NoBorder) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.bordermask = BORDERMASK_LEFT | BORDERMASK_RIGHT |
                  BORDERMASK_TOP | BORDERMASK_BOTTOM;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, BadBorderBitsRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.bordermask = BORDERMASK_LEFT * 2;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, NoTabletBorder) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.tabletmask = BORDERMASK_LEFT | BORDERMASK_RIGHT |
                  BORDERMASK_TOP | BORDERMASK_BOTTOM;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, BadTabletBorderBitsRejected) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.tabletmask = BORDERMASK_LEFT * 2;
  ASSERT_NE(nullptr, outcurses_init(true));
  struct panelreel* pr = panelreel_create(stdscr, &p);
  ASSERT_EQ(nullptr, pr);
  ASSERT_EQ(0, outcurses_stop(true));
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

TEST_F(PanelReelTest, InitWithinSubwin) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  p.loff = 1;
  p.roff = 1;
  p.toff = 1;
  p.boff = 1;
  ASSERT_NE(nullptr, outcurses_init(true));
  EXPECT_EQ(0, clear());
  PANEL* base = make_targwin(stdscr);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = panelreel_create(basew, &p);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_validate(basew, pr));
  ASSERT_EQ(0, panelreel_destroy(pr));
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
  ASSERT_EQ(0, outcurses_stop(true));
}

TEST_F(PanelReelTest, SubwinNoOffsetGeom) {
  if(getenv("TERM") == nullptr){
    GTEST_SKIP();
  }
  panelreel_options p{};
  ASSERT_NE(nullptr, outcurses_init(true));
  EXPECT_EQ(0, clear());
  PANEL* base = make_targwin(stdscr);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = panelreel_create(basew, &p);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_validate(basew, pr));
  ASSERT_EQ(0, panelreel_destroy(pr));
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
  ASSERT_EQ(0, outcurses_stop(true));
}
