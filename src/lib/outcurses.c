#include "outcurses.h"
#include "colors.h"

WINDOW* outcurses_init(bool initcurses){
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

int outcurses_stop(bool stopcurses){
  int ret = 0;
  if(stopcurses){
    if(endwin() != OK){
      fprintf(stderr, "Error during endwin()\n");
    }
  }
  return ret;
}
