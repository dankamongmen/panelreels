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
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

// A callback used to ascertain how many lines are available for a given tablet
// (only the client knows). This will be called with the opaque pointer
// provided by the client to add_tablet() and a maximum number of rows. The
// maximum number of rows will not always be usefully set (i.e., sometimes it
// will be MAXINT), but it can be used to terminate the line calculation
// procedure early when set. Do not consider borders in this return. Negative
// returns are currently invalid, resulting in undefined behavior.
typedef int (*LineCountCB)(const void*);

// A callback used to render lines of information within a tablet. This will be
// called with the tablet's WINDOW, the logical line offset within the tablet,
// the number of lines to render, the number of columns available, and the
// opaque object provided to add_tablet().
typedef int (*DrawLinesCB)(WINDOW*, int, int, int, void*);

typedef struct panelreel_options {
  WINDOW* w;           // ncurses WINDOW we're taking over, non-NULL
  int footerlines;     // leave this many lines alone at bottom, >=0
  int headerlines;     // leave this many lines alone at top, >=0
  int leftcolumns;     // leave this many columns alone on left, >=0
  int rightcolumns;    // leave this many columns alone on right, >=0 
  bool infinitescroll; // is scrolling infinite (can one move down or up
   //  forever, or is an end reached?). if true, 'circular' specifies how to
   //  handle the special case of an incompletely-filled reel.
  bool circular;       // is navigation circular (does moving down from the
   //  last panel move to the first, and vice versa)? only meaningful when
   //  infinitescroll is true. if infinitescroll is false, this must be false.
  LineCountCB linecb;
  DrawLinesCB drawcb;
} panelreel_options;

struct tablet;
struct panelreel;


// Create a panelreel according to the provided specifications. Returns NULL on
// failure.
struct panelreel* create_panelreel(const panelreel_options* popts);

// Add a new tablet to the provided panelreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specifid, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
struct tablet* add_tablet(struct panelreel* pr, struct tablet* after,
                          struct tablet *before, void* opaque);

int del_table(struct panelreel* pr, struct tablet* t);

// Destroy a panelreel allocated with create_panelreel(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
int destroy_panelreel(struct panelreel* preel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
