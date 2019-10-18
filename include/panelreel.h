#ifndef OUTCURSES_PANELREEL
#define OUTCURSES_PANELREEL

#include <ncurses.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// A panelreel is an ncurses window devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time. Each panel has a details depth (how many
// levels of hierarchal data ought be shown).
//
// This structure is amenable both to scrolling gestures and keystrokes.
typedef struct panelreel_options {
  WINDOW* w;           // ncurses WINDOW we're taking over, non-NULL
  int footerlines;     // leave this many lines alone at bottom, >=0
  int headerlines;     // leave this many lines alone at top, >=0
  int leftcolumns;     // leave this many columns alone on left, >=0
  int rightcolumns;    // leave this many columns alone on right, >=0 
  bool circular;       // is navigation circular (does moving down from the
                       //  last panel move to the first, and vice versa)?
} panelreel_options;

struct panelreel;

// Create a panelreel according to the provided specifications. Returns NULL on
// failure.
struct panelreel* create_panelreel(const panelreel_options* popts);

// Destroy a panelreel allocated with create_panelreel(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
int destroy_panelreel(struct panelreel* preel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
