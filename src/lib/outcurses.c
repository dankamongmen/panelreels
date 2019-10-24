#include <outcurses.h>

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

static int
prep_colors(void){
  if(start_color() != OK){
    fprintf(stderr, "Couldn't start color support\n");
    return -1;
  }
  // Use the default terminal colors for COLOR(-1)
  if(use_default_colors()){
    fprintf(stderr, "Warning: couldn't use default terminal colors\n");
  }
  // Defines COLOR_PAIR(0) to be -1, -1 (default terminal colors, see above)
  if(assume_default_colors(-1, -1) != OK){
    fprintf(stderr, "Warning: couldn't assume default colors\n");
  }
  if(COLORS >= 256){
    if(init_8bit_colors()){
      return -1;
    }
  }else if(COLORS >= 16){
    if(init_system_colors()){
      return -1;
    }
  }
  if(COLORS != 16 && COLORS != 256){
    fprintf(stderr, "Warning: unexpected number of colors (%d)\n", COLORS);
  }
  int i;
  for(i = 0 ; i < COLOR_PAIRS ; ++i){
    if(init_extended_pair(i, i % COLORS, -1)){
      fprintf(stderr, "Warning: couldn't initialize colorpair %d\n", i);
    }
  }
  return 0;
}

WINDOW* init_outcurses(bool initcurses){
  WINDOW* scr;

  if(initcurses){
    if((scr = initscr()) == NULL){
      fprintf(stderr, "Couldn't initialize ncurses\n");
      return NULL;
    }
    if(prep_colors()){
      goto error;
    }
    // Sets input to character-at-a-time mode, honoring the terminal driver
    // (thus things like Ctrl+C will not reach us without raw()).
    if(cbreak() != OK){
      fprintf(stderr, "Couldn't set cbreak\n");
    }
    // Inhibits input characters from being printed to the screen.
    if(noecho() != OK){
      fprintf(stderr, "Couldn't disable echo\n");
    }
    // Don't flush output from the tty driver queue upon signal receipt.
    if(intrflush(scr, FALSE) != OK){
      fprintf(stderr, "Couldn't disable flush on interrupt\n");
    }
    // Inhibits the 'enter' key from advancing input/output on the real screen
    // (emitting '\n' still has effect on the virtual screen).
    if(nonl() != OK){
      fprintf(stderr, "Couldn't disable newline\n");
      goto error;
    }
    // Enable the keypad (pass arrow keys etc. through as composed characters).
    if(keypad(scr, TRUE) != OK){
      fprintf(stderr, "Couldn't enable keypad\n");
      goto error;
    }
    // Don't bother moving cursor to refreshed windows, reducing cursor moves.
    // Note: getsyx() is no longer meaningful when this is set.
    if(leaveok(scr, TRUE) != OK){
      fprintf(stderr, "Couldn't disable cursor movement\n");
      goto error;
    }
    // Make the cursor invisible, if supported by the terminal.
    if(curs_set(0) == ERR){
      fprintf(stderr, "Couldn't disable cursor\n");
      goto error;
    }
  }else{
    if((scr = stdscr) == NULL){
      fprintf(stderr, "Couldn't get stdscr (was ncurses initialized?)\n");
      return NULL;
    }
  }
  return scr;

error:
  endwin();
  return NULL;
}

int stop_outcurses(bool stopcurses){
  int ret = 0;
  if(stopcurses){
    if(endwin() != OK){
      fprintf(stderr, "Error during endwin()\n");
    }
  }
  return ret;
}
