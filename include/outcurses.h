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

// Do a palette fade on the specified screen over the course of ms milliseconds.
int fade(WINDOW* w, unsigned ms);

#define PREFIXSTRLEN 7  // Does not include a '\0' (xxx.xxU)
#define BPREFIXSTRLEN 9  // Does not include a '\0' (xxx.xxUi), i == prefix
#define PREFIXFMT "%7s"
#define BPREFIXFMT "%9s"

// Takes an arbitrarily large number, and prints it into a fixed-size buffer by
// adding the necessary SI suffix. Usually, pass a |PREFIXSTRLEN+1|-sized
// buffer to generate up to PREFIXSTRLEN characters.
//
// val: value to print
// decimal: scaling. '1' if none has taken place.
// buf: buffer in which string will be generated
// bsize: size of buffer. ought be at least PREFIXSTRLEN
// omitdec: inhibit printing of all-0 decimal portions
// mult: base of suffix system (almost always 1000 or 1024)
// uprefix: character to print following suffix ('i' for kibibytes basically).
//   only printed if suffix is actually printed (input >= mult).
//
// For full safety, pass in a buffer that can hold the decimal representation
// of the largest uintmax_t plus three (one for the unit, one for the decimal
// separator, and one for the NUL byte).
static inline const char *
genprefix(uintmax_t val, unsigned decimal, char *buf, size_t bsize, 
			int omitdec, unsigned mult, int uprefix){
	const char prefixes[] = "KMGTPEY";
	unsigned consumed = 0;
	uintmax_t dv;

	if(decimal == 0 || mult == 0){
		return NULL;
	}
	dv = mult;
	while((val / decimal) >= dv && consumed < strlen(prefixes)){
		dv *= mult;
		++consumed;
		if(UINTMAX_MAX / dv < mult){ // watch for overflow
			break;
		}
	}
	if(dv != mult){ // if consumed == 0, dv must equal mult
		dv /= mult;
		val /= decimal;
		// Remainder is val % dv; we want a percentage as scaled integer
		unsigned remain = (val % dv) * 100 / dv;
		if(remain || omitdec == 0){
			// FIXME we throw the % 100 on remain to avoid a
			// format-truncation warning. remain ought always be
			// less than 100, since integer division goes to 0.
			snprintf(buf, bsize,"%ju.%02u%c%c",
					val / dv,
					remain % 100,
					prefixes[consumed - 1],
					uprefix);
		}else{
			snprintf(buf, bsize, "%ju%c%c", val / dv,
						prefixes[consumed - 1], uprefix);
		}
	}else{
		if(val % decimal || omitdec == 0){
			snprintf(buf, bsize, "%ju.%02ju", val / decimal, val % decimal);
		}else{
			snprintf(buf, bsize, "%ju", val / decimal);
		}
	}
	return buf;
}

// Mega, kilo, gigabytes
static inline const char *
qprefix(uintmax_t val, unsigned decimal, char *buf, size_t bsize, int omitdec){
	return genprefix(val, decimal, buf, bsize, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes
static inline const char *
bprefix(uintmax_t val, unsigned decimal, char *buf, size_t bsize, int omitdec){
	return genprefix(val, decimal, buf, bsize, omitdec, 1024, 'i');
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif
