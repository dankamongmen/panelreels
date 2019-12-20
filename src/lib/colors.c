#include "outcurses.h"
#include "colors.h"

// Ideally, the terminal supports at least 256 independent colors and at least
// 512 independent color pairs. Colors are made up of 3(4?) 256-bit channels,
// and comprise the palette. Color pairs are made up of a foreground and
// background color, both of which index into the palette. The color -1 is the
// default color for its context (foreground or background) as inherited from
// the terminal, via the magic of assume_default_colors().
//
// We set up the color pairs so that each block of 256 pairs is the 256 colors
// as the foreground, with one of the 256 as the background.
int prep_colors(void){
  if(start_color() != OK){
    fprintf(stderr, "Couldn't start color support\n");
    return -1;
  }
  // Use the default terminal colors for COLOR(-1), then defines COLOR_PAIR(0)
  // to be -1, -1 (default terminal colors). assume_default_colors(-1, -1)
  // encompasses use_default_colors().
  if(assume_default_colors(-1, -1) != OK){
    fprintf(stderr, "Warning: couldn't assume default colors\n");
  }
  int bg;
  int pair = 0;
  for(bg = -1 ; bg < COLORS - 1 ; ++bg){
    int i;
    for(i = 0 ; i < COLORS ; ++i){
      if(pair >= COLOR_PAIRS){
        break;
      }
      if(pair){
        if(init_extended_pair(pair, i, bg)){
          fprintf(stderr, "Warning: couldn't initialize colorpair %d/%d\n", pair, COLOR_PAIRS);
          goto err;
        }
      }
      ++pair;
    }
  }
err:
  return 0;
}
