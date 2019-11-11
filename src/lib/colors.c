#include "outcurses.h"
#include "colors.h"

static const int MAXHUE = 1000;

#define RGB(r, g, b) (r) * MAXHUE / 256, (g) * MAXHUE / 256, (b) * MAXHUE / 256

// RGB values for system colors taken from Windows 10 Console. I don't use
// Windows or anything, but these do look nice, and Redmond certainly outspends
// the Free world on ... color consultants(?).
static int
init_system_colors(void){
  int ret = 0;
  ret |= init_extended_color(COLOR_BLACK, RGB(0, 0, 0));
  ret |= init_extended_color(COLOR_RED, RGB(197, 15, 31));
  ret |= init_extended_color(COLOR_GREEN, RGB(19, 161, 14));
  ret |= init_extended_color(COLOR_YELLOW, RGB(193, 156, 0));
  ret |= init_extended_color(COLOR_BLUE, RGB(0, 55, 218));
  ret |= init_extended_color(COLOR_MAGENTA, RGB(136, 23, 152));
  ret |= init_extended_color(COLOR_CYAN, RGB(58, 150, 221));
  ret |= init_extended_color(COLOR_WHITE, RGB(204, 204, 204));
  if(ret){
    fprintf(stderr, "Error initializing system colors\n");
  }
  return ret;
}

static int
init_8bit_colors(void){
  // Prepare our palette. Use the system palette for the first 16 colors, then
  // build a 6x6x6 RGB color cube, and finally a greyscale ramp.
  if(init_system_colors()){
    return -1;
  }
  int idx = 16;
  int r, g, b;
  const int RGBSTEP = MAXHUE / 6;
  for(r = 0 ; r < 6 ; ++r){
    for(g = 0 ; g < 6 ; ++g){
      for(b = 0 ; b < 6 ; ++b){
        if(init_extended_color(idx++, r * RGBSTEP, g * RGBSTEP, b * RGBSTEP)){
          fprintf(stderr, "Error initializing color %d\n", idx - 1);
          return -1;
        }
      }
    }
  }
  // We explicitly leave out black and white, so take bigger steps
  const int GREYSTEP = MAXHUE / (256 - idx + 2);
  int grey = 0;
  while(idx < 256){
    grey += GREYSTEP;
    if(init_extended_color(idx++, grey, grey, grey)){
      fprintf(stderr, "Error initializing color %d\n", idx - 1);
      return -1;
    }
  }
  return 0;
}

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
  if(COLOR_PAIRS < COLORS){
    fprintf(stderr, "Error: fewer COLOR_PAIRS than COLORS\n");
    return -1;
  }
  // Use the default terminal colors for COLOR(-1), then defines COLOR_PAIR(0)
  // to be -1, -1 (default terminal colors). assume_default_colors(-1, -1)
  // encompasses use_default_colors().
  if(assume_default_colors(-1, -1) != OK){
    fprintf(stderr, "Warning: couldn't assume default colors\n");
  }
  if(COLORS >= 256){
    if(init_8bit_colors()){ // calls init_system_colors()
      return -1;
    }
  }else if(COLORS >= 16){
    if(init_system_colors()){
      return -1;
    }
  }
  // linux console (unless TERM is linux-16color) is only 8/256. fbterm is
  // 256/32768. old rxvt has 88(?).
  if(COLORS != 16 && COLORS != 256){
    fprintf(stderr, "Warning: unexpected number of colors (%d)\n", COLORS);
  }
  int bg;
  for(bg = -1 ; bg < COLORS - 1 ; ++bg){
    int i;
    for(i = 0 ; i < COLORS ; ++i){
      int pair = i + (bg + 1) * COLORS;
      if(pair >= COLOR_PAIRS){
        break;
      }
      if(pair != 0){
        if(init_extended_pair(pair, i, bg)){
          fprintf(stderr, "Warning: couldn't initialize colorpair %d\n", i);
        }
      }
    }
  }
  return 0;
}
