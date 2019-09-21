#ifndef OUTCURSES_OUTCURSES
#define OUTCURSES_OUTCURSES

#include <term.h>
#include <panel.h>
#include <ncurses.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {

namespace Outcurses {
#endif

// Initialize the library. Will initialize ncurses unless initcurses is false.
// You are recommended to call init_outcurses prior to interacting with ncurses,
// and to set initcurses to true.
int init_outcurses(bool initcurses);

// Stop the library. If stopcurses is true, endwin() will be called, (ideally)
// restoring the screen and cleaning up ncurses.
int stop_outcurses(bool stopcurses);

// Do a palette fade on the specified screen over the course of sec seconds.
int fade(WINDOW* w, unsigned sec);

#ifdef __cplusplus
} // namespace Outcurses

} // extern "C"
#endif

#endif
