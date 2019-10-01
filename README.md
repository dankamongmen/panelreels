# outcurses
by Nick Black <dankamongmen@gmail.com>

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/outcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/outcurses)

Outcurses is an extension of NCURSES by Thomas Dickey et al, in the spirit of
the Panels, Menu, and Forms extensions. It does not require patching the
NCURSES source. You might need to rebuild it to take advantage of all features,
but this is unlikely if you're using your distro's NCURSES package. Make sure
that the "wide" version (NCURSESW) is available.

* Building

 - NCURSES 6.1+ with wide character support is required.
 - GoogleTest 1.9.0+ is required.
   - As of 2019-10, GoogleTest 1.9.0 has not yet been released. Debian ships
	 a prerelease. Arch is lacking. You need a build with `GTEST_SKIP`.
 - CMake 3.16+ is required on Arch. You can get by with 3.13 on Debian. Chant
   the standard incantations, and form your parentheses of salt.

## Panel wheels
Panel wheels implement a dynamic collection of collapsible panels within a
provided Ncurses WINDOW *. The panels can be independently grown and shrunk,
any number of panels can be created, and panels and disappear and reappear. At
any time, zero or one panels have the focus (zero only if there exist no
panels). Visible space is allocated to the focused panel up through its needs,
if possible. If any space remains, it is allocated to other panels based in
order of their distance from the focused panel, until exhausted.

Arbitrary absolute selection of a panel is supported, as are "next" and
"previous" relative selections (i.e., search, up/left, and down/right). The
panels can be thought of as pasted to the outside of a cylinder (the "wheel")
for the purposes of selecting the "next" or "previous" panel. Note that this
means, if the visual space is not exhausted, that selecting "next" or
"previous" at the lower or upper boundary respectively will permute the
displayed panels. This is so that the user interface is consistent across all
possible panel dynamics.

## genprefix()

genprefix() reduces a (potentially very large) `uintmax_t` to a string of fixed
width, employing a metrix suffix when appropriate. The base should be specified
as 1000 for most uses, or 1024 for digital information ('kibibytes' etc.). The
uprefix is an additional character printed following the suffix, if and only if
the suffix is printed. If 1024 is the base, it makes sense to use 'i' as a
uprefix. If omitdecimal is not zero, mantissae consisting entirely of zeroes
will not be printed. Rounding is always towards zero.

The output will be not more than PREFIXSTRLEN+1 (8) characters for a base of
1000, or BPREFIXSTRLEN+1 (10) for a base greater than 1000 with a uprefix
(these values include the trailing NUL byte).

Examples:

```C
char out[PREFIXSTRLEN + 1]
genprefix(999, 1, out, 0, 1000, '\0');         ---> "999"
genprefix(999, 1, out, 0, 1024, 'i');          ---> "999"
genprefix(1000, 1, out, 1, 1000, '\0');         ---> "1K"
genprefix(1000, 1, out, 0, 1024, 'i');          ---> "1000"
genprefix(1023, 1, out, 0, 1024, 'i');          ---> "1023"
genprefix(1024, 1, out, 0, 1024, 'i');          ---> "1.00Ki"
genprefix(99999, 1, out, 0, 1000, '\0');       ---> "999.99K"
genprefix(99999, 1, out, 1, 1000, '\0');       ---> "999.99K"
genprefix(100000, 1, out, 0, 1000, '\0');      ---> "100.00K"
genprefix(100000, 1, out, 1, 1000, '\0');      ---> "100K"
genprefix(100000, 1, out, 1, 1024, 'i');       ---> "97.65Ki"
genprefix(1u << 17, 1, out, 1, 1000, '\0');    ---> "131.07K"
genprefix(1u << 17, 1, out, 0, 1024, 'i');     ---> "128.00Ki"
genprefix(1u << 17, 1, out, 1, 1024, 'i');     ---> "128Ki"
genprefix(INTMAX_MAX, 1, out, 1, 1000, '\0');  ---> "9.22E"
genprefix(INTMAX_MAX, 1, out, 1, 1024, 'i');   ---> "8Ei"
genprefix(UINTMAX_MAX, 1, out, 1, 1000, '\0'); ---> "18.44E"
genprefix(UINTMAX_MAX, 1, out, 1, 1024, 'i');  ---> "15.99Ei"
```

I know this doesn't have anything to do with ncurses, but eh, it's output.
