#ifndef OUTCURSES_OUTCURSES
#define OUTCURSES_OUTCURSES

#include <term.h>
#include <string.h>
#include <ncurses.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the library. Will initialize ncurses unless initcurses is false.
// You are recommended to call init_outcurses prior to interacting with ncurses,
// and to set initcurses to true.
int init_outcurses(bool initcurses);

// Stop the library. If stopcurses is true, endwin() will be called, (ideally)
// restoring the screen and cleaning up ncurses.
int stop_outcurses(bool stopcurses);

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

// Do a palette fade on the specified screen over the course of ms milliseconds.
int fade(WINDOW* w, unsigned ms);

#define PREFIXSTRLEN 7  // Does not include a '\0' (xxx.xxU)
#define BPREFIXSTRLEN 9  // Does not include a '\0' (xxxx.xxUi), i == prefix
#define PREFIXFMT "%7s"
#define BPREFIXFMT "%9s"

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
static inline const char *
enmetric(uintmax_t val, unsigned decimal, char *buf, int omitdec,
         unsigned mult, int uprefix){
  const char prefixes[] = "KMGTPEZY"; // 10^21-1 encompasses 2^64-1
  unsigned consumed = 0;
  uintmax_t dv;

  if(decimal == 0 || mult == 0){
    return NULL;
  }
  dv = mult;
  // FIXME verify that input < 2^89, wish we had static_assert() :/
  while((val / decimal) >= dv && consumed < strlen(prefixes)){
    dv *= mult;
    ++consumed;
    if(UINTMAX_MAX / dv < mult){ // near overflow--can't scale dv again
      break;
    }
  }
  if(dv != mult){ // if consumed == 0, dv must equal mult
    if(val / dv > 0){
      ++consumed;
    }else{
      dv /= mult;
    }
    val /= decimal;
    // Remainder is val % dv, but we want a percentage as scaled integer.
    // Ideally we would multiply by 100 and divide the result by dv, for
    // maximum accuracy (dv might not be a multiple of 10--it is not for
    // 1,024). That can overflow with large 64-bit values, but we can first
    // divide both sides by mult, and then scale by 100.
    if(omitdec && (val % dv) == 0){
      sprintf(buf, "%ju%c%c", val / dv,
          prefixes[consumed - 1], uprefix);
    }else{
      uintmax_t remain = (dv == mult) ?
                remain = (val % dv) * 100 / dv :
                ((val % dv) / mult * 100) / (dv / mult);
      sprintf(buf, "%ju.%02ju%c%c",
          val / dv,
          remain,
          prefixes[consumed - 1],
          uprefix);
    }
  }else{ // unscaled output, consumed == 0, dv == mult
    if(omitdec && val % decimal == 0){
      sprintf(buf, "%ju", val / decimal);
    }else{
      sprintf(buf, "%ju.%02ju", val / decimal, val % decimal);
    }
  }
  return buf;
}

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
