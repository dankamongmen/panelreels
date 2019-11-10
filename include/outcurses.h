#ifndef OUTCURSES_OUTCURSES
#define OUTCURSES_OUTCURSES

#include <panel.h>
#include <ncurses.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the library. Will initialize ncurses suitably for fullscreen mode
// unless initcurses is false. You are recommended to call init_outcurses prior
// to interacting with ncurses, and to set initcurses to true.
WINDOW* init_outcurses(bool initcurses);

// Stop the library. If stopcurses is true, endwin() will be called, (ideally)
// restoring the screen and cleaning up ncurses.
int stop_outcurses(bool stopcurses);

// A set of RGB color components
typedef struct outcurses_rgb {
  int r, g, b;
} outcurses_rgb;

// Do a palette fade on the specified screen over the course of ms milliseconds.
// FIXME allow colorpair range to be specified for partial-screen fades
int fadeout(WINDOW* w, unsigned ms);

// count ought be COLORS to retrieve the entire palette. palette is a
// count-element array of rgb values. if maxes is non-NULL, it points to a
// single rgb triad, which will be filled in with the maximum components found.
// if zeroout is set, the palette will be set to all 0s. to fade in, we
// generally want to prepare the screen using a zerod-out palette, then
// initiate the fadein targeting the true palette.
int retrieve_palette(int count, outcurses_rgb* palette, outcurses_rgb* maxes,
                     bool zeroout);

// fade in to the specified palette
// FIXME can we not just generalize this plus fadeout to 'fadeto'?
int fadein(WINDOW* w, int count, const outcurses_rgb* palette, unsigned ms);

// Restores a palette through count colors.
int set_palette(int count, const outcurses_rgb* palette);

// A panelreel is an ncurses window devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

enum bordermaskbits {
  BORDERMASK_TOP    = 0x1,
  BORDERMASK_RIGHT  = 0x2,
  BORDERMASK_BOTTOM = 0x4,
  BORDERMASK_LEFT   = 0x8,
};

typedef struct panelreel_options {
  // require this many rows and columns (including borders) for the panelreel
  // to be displayed. otherwise, a message will be displayed stating that a
  // larger terminal is necessary, and input will be queued. if 0, no minima
  // will be enforced. may not be negative.
  int min_supported_cols;
  int min_supported_rows;
  // is scrolling infinite (can one move down or up forever, or is an end
  // reached?). if true, 'circular' specifies how to handle the special case of
  // an incompletely-filled reel.
  bool infinitescroll;
  // is navigation circular (does moving down from the last panel move to the
  // first, and vice versa)? only meaningful when infinitescroll is true. if
  // infinitescroll is false, this must be false.
  bool circular;
  // outcurses can draw a border around the panelreel, and also around the
  // component tablets. inhibit borders by setting all valid bits in the masks.
  // partially inhibit borders by setting individual bits in the masks. the
  // appropriate attr and pair values will be used to style the borders.
  // focused and non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn. taken from bordermaskbits
  attr_t borderattr;   // attributes used for panelreel border, no color!
  int borderpair;      // extended color pair for panelreel border
  unsigned tabletmask; // bitfield; same as bordermask but for tablet borders
  attr_t tabletattr;   // attributes used for tablet borders, no color!
  int tabletpair;      // extended color pair for tablet borders
  attr_t focusedattr;  // attributes used for focused tablet borders, no color!
  int focusedpair;     // extended color pair for focused tablet borders
} panelreel_options;

struct tablet;
struct panelreel;

// Create a panelreel according to the provided specifications. Returns NULL on
// failure. w must be a valid WINDOW*, to which offsets are relative. Note that
// there might not be enough room for the specified offsets, in which case the
// panelreel will be clipped on the bottom and right. A minimum number of rows
// and columns can be enforced via popts.
struct panelreel* create_panelreel(WINDOW* w, const panelreel_options* popts,
                                   int toff, int roff, int boff, int loff);

// Tablet draw callback, provided a PANEL (from which a WINDOW may be derived),
// the first column that may be used, the first row that may be used, the first
// column that may not be used, the first row that may not be used, and a bool
// indicating whether output ought be clipped at the top (true) or bottom
// (false). The curry provided to add_tablet() will also be provided. Rows and
// columns are zero-indexed, and both are relative to the PANEL's WINDOW.
//
// Regarding clipping: it is possible that the tablet is only partially
// displayed on the screen. If so, it is either partially present on the top of
// the screen, or partially present at the bottom. In the former case, the top
// is clipped (cliptop will be true), and output ought start from the end. In
// the latter case, cliptop is false, and output ought start from the beginning.
typedef void (*tabletcb)(PANEL* p, int begx, int begy, int maxx, int maxy,
                         bool cliptop, void* curry);

// Add a new tablet to the provided panelreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specifid, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
struct tablet* add_tablet(struct panelreel* pr, struct tablet* after,
                          struct tablet *before, tabletcb cb, void* opaque);

// Return the number of tablets.
int panelreel_tabletcount(const struct panelreel* pr);

// Delete the tablet specified by t from the panelreel specified by pr. Returns
// -1 if the tablet cannot be found.
int del_tablet(struct panelreel* pr, struct tablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
int del_active_tablet(struct panelreel* pr);

// Redraw the panelreel in its entirety, for instance after
// clearing the screen due to external corruption, or a SIGWINCH.
int panelreel_redraw(const struct panelreel* pr);

// Destroy a panelreel allocated with create_panelreel(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
int destroy_panelreel(struct panelreel* pr);

#define PREFIXSTRLEN 7  // Does not include a '\0' (xxx.xxU)
#define IPREFIXSTRLEN 8 //  Does not include a '\0' (xxxx.xxU)
#define BPREFIXSTRLEN 9  // Does not include a '\0' (xxxx.xxUi), i == prefix

// A bit of the nasties here to stringize our preprocessor tokens just now
// #defined, making them usable as printf(3) specifiers.
#define STRHACK1(x) #x
#define STRHACK2(x) STRHACK1(x)
#define PREFIXFMT "%" STRHACK2(PREFIXSTRLEN) "s"
#define IPREFIXFMT "%" STRHACK2(IPREFIXSTRLEN) "s"
#define BPREFIXFMT "%" STRHACK2(BPREFIXSTRLEN) "s"

// Takes an arbitrarily large number, and prints it into a fixed-size buffer by
// adding the necessary SI suffix. Usually, pass a |[B]PREFIXSTRLEN+1|-sized
// buffer to generate up to [B]PREFIXSTRLEN characters. The characteristic can
// occupy up through |mult-1| characters (3 for 1000, 4 for 1024). The mantissa
// can occupy either zero or two characters.
//
// Floating-point is never used, because an IEEE758 double can only losslessly
// represent integers through 2^53-1.
//
// 2^64-1 is 18446744073709551615, 18.45E(xa). KMGTPEZY thus suffice to handle
// a 89-bit uintmax_t. Beyond Z(etta) and Y(otta) lie lands unspecified by SI.
//
// val: value to print
// decimal: scaling. '1' if none has taken place.
// buf: buffer in which string will be generated
// omitdec: inhibit printing of all-0 decimal portions
// mult: base of suffix system (almost always 1000 or 1024)
// uprefix: character to print following suffix ('i' for kibibytes basically).
//   only printed if suffix is actually printed (input >= mult).
const char *enmetric(uintmax_t val, unsigned decimal, char *buf, int omitdec,
                     unsigned mult, int uprefix);

// Mega, kilo, gigabytes. Use PREFIXSTRLEN + 1.
static inline const char *
qprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char *
bprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1024, 'i');
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif
