# [outcurses](https://nick-black.com/dankwiki/index.php/Outcurses)
by Nick Black <dankamongmen@gmail.com>

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/outcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/outcurses)

Outcurses is middleware atop NCURSES by Thomas Dickey et al, in the spirit of
the Panels, Menu, and Forms extensions. Its capabilities include "panelreels",
palette fades, and other miscellany.

## Building

- NCURSES 6.1+ with wide character support is required.
- GoogleTest 1.9.0+ is required. As of 2019-10, GoogleTest 1.9.0 has not yet
    been released. Debian ships a prerelease. Arch is lacking. You need a build
    with `GTEST_SKIP`.
- CMake 3.16+ is required on Arch. You can get by with 3.13 on Debian. Chant
    the standard incantations, and form your parentheses of salt.

## Getting started

`init_outcurses()` must be called before calling most functions in outcurses.
If you have not initialized ncurses yourself, pass `true`, and
`init_outcurses()` will do so. Otherwise, pass `false` and ncurses proper will
be left unmolested.

When you're done, call `stop_outcurses()` with `true` to have it tear down
itself and ncurses. If you intend to close down ncurses yourself, pass `false`.

Outcurses is thread-safe so long as multiple threads never call into it
concurrently (to the degree that the underlying ncurses is thread-safe).

## Outcurses and SIGWINCH

Outcurses does not explicitly install any SIGWINCH (SIGnal WIndow CHange)
handler. Ncurses itself will install a SIGWINCH handler *if the signal is
currently ignored* (the default). The effect of said handler is twofold:

* The next call to `wgetch()` et al will return `KEY_RESIZE`, and
* The next call to either the `wgetch()` family or `doupdate()` will invoke
    `resizeterm()`, prior to returning `KEY_RESIZE` (in the former case).

Active panelreels ought be redrawn with `panelreel_redraw()`.

## Outcurses and colors

If told to initialize ncurses (by providing `true` to `init_outcurses`),
outcurses will initialize up to 256 colors and colorpairs. The first 16 colors
will maintain their ANSI definitions. The colors between 16 and 231, inclusive,
will be assigned an RGB 6x6x6 color cube, while the final 24 colors get a
linear greyscale ramp (not including black or white). Each color, along with
a neutral background, will be assigned to the corresponding colorpair. You may
freely redefine the palette following `init_outcurses`.

## Panelreels
The panelreel is a UI abstraction supported by outcurses in which
dynamically-created and -destroyed toplevel entities (referred to as tablets)
are arranged in a torus (circular loop), allowing for infinite scrolling
(infinite scrolling can be disabled, resulting in a line segment rather than a
torus). This works naturally with keyboard navigation, mouse scrolling wheels,
and touchpads (including the capacitive touchscreens of modern cell phones).
The "panel" comes from the underlying ncurses objects (each entity corresponds
to a single panel) and the "reel" from slot machines. A panelreel initially has
no tablets; at any given time thereafter, it has zero or more tablets, and if
there is at least one tablet, one tablet is focused (and on-screen). If the
last tablet is removed, no tablet is focused. A tablet can support navigation
within the tablet, in which case there is an in-tablet focus for the focused
tablet, which can also move among elements within the tablet.

The panelreel object tracks the size of the screen, the size, number,
information depth, and order of tablets, and the focuses. It also draws the
optional borders around tablets and the optional border of the reel itself. It
knows nothing about the actual content of a tablet, save the number of lines it
occupies at each information depth. The typical control flow is that an
application receives events (from the UI or other event sources), and calls
into outcurses saying e.g. "Tablet 2 now has 40 valid lines of information".
Outcurses might then call back into the application, asking it to draw some
line(s) from some tablet(s) at some particular coordinate of that tablet's
panel. Finally, control returns to the application, and the cycle starts anew.

Each tablet might be wholly, partially, or not on-screen. Outcurses always
places as much of the focused tablet as is possible on-screen (if the focused
tablet has more lines than the actual reel does, it cannot be wholly on-screen.
In this case, the focused subelements of the tablet are always on-screen). The
placement of the focused tablet depends on how it was reached (when moving to
the next tablet, offscreen tablets are brought onscreen at the bottom. When
moving to the previous tablet, offscreen tablets are brought onscreen at the
top. When moving to an arbitrary tablet which is neither the next nor previous
tablet, it will be placed in the center).

The controlling application can, at any time,

* Insert a new tablet somewhere in the reel (possibly off-screen)
* Delete a (possibly off-screen) tablet from the reel
* Change focus to the next or previous tablet, bringing it on-screen if it is off
* Change focus to some arbitrary other tablet, bringing it on-screen if it is off
* Expand or collapse the information depth of a tablet
* Change the content of a tablet, updating it if it is on-screen
  * Remove content from a tablet, possibly resizing it, and possibly changing focus within the tablet
  * Add content to the tablet, possibly resizing it, and possibly creating focus within the tablet
* Navigate within the focused tablet
* Create or destroy new panels atop the panelreel
* Indicate that the screen has been resized or needs be redrawn

A special case arises when moving among the tablets of a reel having multiple
tablets, all of which fit entirely on-screen, and infinite scrolling is in use.
Normally, upon moving to the next tablet from the bottommost tablet, the
(offscreen) next tablet is pulled up into the bottom of the reel (the reverse
is true when moving to the previous tablet from the topmost). When all tablets
are onscreen with infinite scrolling, there are two possibilities: either the
focus scrolls (moving from the bottom tablet to the top tablet, for instance),
or the reel scrolls (preserving order among the tablets, but changing their
order on-screen). In this latter case, moving to the next tablet from the
bottommost tablet results in the tablet which is gaining focus being brought to
the bottom of the screen from the top, and all other tablets moving up on the
screen. Moving to the previous tablet from the topmost tablet results in the
bottommost tablet moving to the top of the screen, and all other tablets moving
down. This behavior matches the typical behavior precisely, and avoids a rude
UI discontinuity when the tablets grow to fill the entire screen (or shrink to
not fill it). If it is not desired, however, scrolling of focus can be
configured instead.

### Panelreel examples

Let's say we have a screen of 11 lines, and 3 tablets of one line each. Both
a screen border and tablet borders are in use. The tablets are A, B, and C.
No gap is in use between tablets. Xs indicate focus. If B currently has focus,
and the next tablet is selected, the result would be something like:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |   A   | |                         | |   A   | |
 | --------- |                         | --------- |
 | --------- | ---- "next tablet" ---> | --------- |
 | |XX B XX| |                         | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |XX C XX| |
 | --------- |                         | --------- |
 -------------                         -------------
```

If instead the previous tablet had been selected, we would of course get:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |   A   | |                         | |XX A XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |XX B XX| |                         | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

If A instead has the focus, choosing the "next tablet" is trivial: the tablets
do not change, and focus shifts to B. If we choose the "previous tablet", there
are three possibilities:

* Finite scrolling: No change. The tablets stay in place. A remains focused.

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |XX A XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |     (finite scroll)     | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

* Infinite scrolling with rotation: Focus shifts to C, which moves to the top:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |XX C XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |  (infinite scroll with  | |   A   | |
 | --------- |        rotation)        | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   B   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

* Infinite scrolling with focus rotation: Focus shifts to C, and moves to the bottom:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |   A   | |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |  (infinite scroll with  | |   B   | |
 | --------- |     focus rotation)     | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |XX C XX| |
 | --------- |                         | --------- |
 -------------                         -------------
```

Now imagine us to have the same 3 tablets, but each is now 4 lines. It is
impossible to have two of these tablets wholly onscreen at once, let alone all
three. If we started with A focused and at the top, the result after all three
tablets have grown will be:

```
 -------------                         -------------
 | --------- |                         | --------- | A remains at the top, and
 | |XX A XX| |                         | |XXXXXXX| | is wholly on-screen. B is
 | --------- |                         | |XX A XX| | below it, but we can show
 | --------- | ---- "grow tablet" ---> | |XXXXXXX| | only the first two lines.
 | |   B   | |       A (focused)       | |XXXXXXX| | C has been pushed
 | --------- |                         | --------- | off-screen.
 | --------- |                         | --------- |
 | |   C   | |                         | |       | |
 | --------- |                         | |   B   | |
 -------------                         -------------
```

When a tablet is enlarged, it grows towards the nearest boundary, unless that
would result in the focused tablet being moved, in which case the growing
tablet instead grows in the other direction (if the tablet is in the middle
of the screen exactly, it grows down). There is one exception to this rule: if
the tablets are not making full use of the screen, growth is always down (the
screen is always filled from the top), even if it moves the focused tablet.

A 12-line screen has three tablets: A (2 lines), B (1 line), C (1 line), filling
the screen exactly. B is focused, and grows two lines:

```
 -------------                         -------------
 | --------- |                         | --------- | B grows down, since it is
 | |   A   | |                         | |   A   | | closer to the bottom (3
 | |       | |                         | |       | | lines) than the top (4
 | --------- | ---- "grow tablet" ---> | --------- | lines). C is pushed almost
 | --------- |       B (focused)       | --------- | entirely off-screen. A is
 | |XX B XX| |                         | |XXXXXXX| | untouched.
 | --------- |                         | |XX B XX| |
 | --------- |                         | |XXXXXXX| |
 | |   C   | |                         | --------- |
 | --------- |                         | --------- |
 -------------                         -------------
```

Starting with the same situation, A grows by 2 lines instead:

```
 -------------                         -------------
 | --------- |                         | |       | | A grows up. It would have
 | |   A   | |                         | |   A   | | grown down, but that would
 | |       | |                         | |       | | have moved B, which has
 | --------- | ---- "grow tablet" ---> | --------- | the focus. B and C remain
 | --------- |     A (not focused)     | --------- | where they are; A moves
 | |XX B XX| |                         | |XX B XX| | partially off-screen.
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

If we started with the same situation, and B grew by 7 lines, it would first
push C entirely off-screen (B would then have four lines of text), and then
push A off-screen. B would then have eight lines of text, the maximum on a
12-line screen with both types of borders.

## fade()

Palette fades in the terminal! Works for any number of supported colors, but
*does not* affect the "default colors" (ncurses color number -1). Takes as its
argument a number of milliseconds, which will be the target for a complete
fade. Blocks, and adapts to timing irregularities (i.e. smoothly takes into
account early or late wakeups). Upon completion, restores the palette to that
in use upon entry.

## enmetric()

enmetric() reduces a (potentially very large) `uintmax_t` to a string of fixed
width, employing a metrix suffix when appropriate. `base` should be specified
as 1000 for most uses, or 1024 for digital information ('kibibytes' etc.). The
uprefix is an additional character printed following the suffix, if and only if
the suffix is printed. If 1024 is the base, it makes sense to use 'i' as a
uprefix. If `omitdecimal` is not zero, a mantissa consisting entirely of zeroes
will not be printed if the value is indeed precise. Rounding is always towards
zero.

The output will be not more than PREFIXSTRLEN+1 (8) characters for a base of
1000, or BPREFIXSTRLEN+1 (10) for a base greater than 1000 with a uprefix
(these values include the trailing NUL byte).

Examples:

```C
char out[PREFIXSTRLEN + 1]
enmetric(999, 1, out, 0, 1000, '\0');           ---> "999.00"
enmetric(999, 1, out, 1, 1000, '\0');           ---> "999"
enmetric(999, 1, out, 0, 1024, 'i');            ---> "999.00"
enmetric(999, 1, out, 1, 1024, 'i');            ---> "999.00"
enmetric(1000, 1, out, 0, 1000, '\0');          ---> "1.00K"
enmetric(1000, 1, out, 1, 1000, '\0');          ---> "1K"
enmetric(1000, 1, out, 0, 1024, 'i');           ---> "1000.00"
enmetric(1023, 1, out, 0, 1024, 'i');           ---> "1023.00"
enmetric(1024, 1, out, 0, 1024, 'i');           ---> "1.00Ki"
enmetric(1024, 1, out, 1, 1024, 'i');           ---> "1Ki"
enmetric(1025, 1, out, 0, 1024, 'i');           ---> "1.00Ki"
enmetric(99999, 1, out, 0, 1000, '\0');         ---> "999.99K"
enmetric(99999, 1, out, 1, 1000, '\0');         ---> "999.99K"
enmetric(100000, 1, out, 0, 1000, '\0');        ---> "100.00K"
enmetric(100000, 1, out, 1, 1000, '\0');        ---> "100K"
enmetric(100000, 1, out, 1, 1024, 'i');         ---> "97.65Ki"
enmetric(1u << 17, 1, out, 1, 1000, '\0');      ---> "131.07K"
enmetric(1u << 17, 1, out, 0, 1024, 'i');       ---> "128.00Ki"
enmetric(1u << 17, 1, out, 1, 1024, 'i');       ---> "128Ki"
enmetric(INTMAX_MAX, 1, out, 1, 1000, '\0');    ---> "9.22E"
enmetric(INTMAX_MAX + 1, 1, out, 1, 1000, '\0');---> "9.22E"
enmetric(INTMAX_MAX, 1, out, 1, 1024, 'i');     ---> "7.99Ei"
enmetric(INTMAX_MAX + 1, 1, out, 1, 1024, 'i'); ---> "8Ei"
enmetric(UINTMAX_MAX, 1, out, 1, 1000, '\0');   ---> "18.44E"
enmetric(UINTMAX_MAX, 1, out, 1, 1024, 'i');    ---> "15.99Ei"
```

I know this doesn't have anything to do with ncurses, but eh, it's output.
