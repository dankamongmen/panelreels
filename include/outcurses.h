#ifndef OUTCURSES_OUTCURSES
#define OUTCURSES_OUTCURSES

#include <term.h>
#include <panel.h>
#include <ncurses.h>

#ifdef __cplusplus
extern "C" {

namespace Outcurses {
#endif

// Do a palette fade on the active screen over the course of sec seconds.
int fade(unsigned sec);

#ifdef __cplusplus
} // namespace Outcurses

} // extern "C"
#endif

#endif
