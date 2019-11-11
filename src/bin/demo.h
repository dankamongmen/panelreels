#ifndef OUTCURSES_DEMO
#define OUTCURSES_DEMO

#include <outcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FADE_MILLISECONDS 1000

int widecolor_demo(WINDOW* w);
struct panelreel* panelreel_demo(WINDOW* w);

#ifdef __cplusplus
}
#endif

#endif
