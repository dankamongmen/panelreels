#include <stdlib.h>
#include <pthread.h>
#include <outcurses.h>
#include "demo.h"

typedef struct tabletctx {
  pthread_t tid;
  struct panelreel* pr;
  struct tablet* t;
  unsigned lines;
  int cpair;
  struct tabletctx* next;
  pthread_mutex_t lock;
} tabletctx;

static int
tabletdraw(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
           void* vtabletctx){
  tabletctx* tctx = vtabletctx;
  int x, y;
  WINDOW* w = panel_window(p);
  for(y = begy ; y < maxy ; ++y){
    wmove(w, y, begx);
    cchar_t cch;
    setcchar(&cch, L"X", A_NORMAL, 0, &tctx->cpair);
    for(x = begx ; x < maxx ; ++x){
      wadd_wch(w, &cch);
    }
  }
  return maxy - begy;
}

// Each tablet has an associated thread which will periodically send update
// events for its tablet.
static void*
tablet_thread(void* vtabletctx){
  tabletctx* tctx = vtabletctx;
  return tctx;
}

static tabletctx*
new_tabletctx(struct panelreel* pr){
  tabletctx* tctx = malloc(sizeof(*tctx));
  if(tctx == NULL){
    return NULL;
  }
  pthread_mutex_init(&tctx->lock, NULL);
  tctx->pr = pr;
  tctx->lines = random() % 5; // FIXME a nice gaussian would be swell
  tctx->cpair = random() % COLORS;
  if((tctx->t = add_tablet(pr, NULL, NULL, tabletdraw, tctx)) == NULL){
    pthread_mutex_destroy(&tctx->lock);
    free(tctx);
    return NULL;
  }
  if(pthread_create(&tctx->tid, NULL, tablet_thread, tctx)){
    pthread_mutex_destroy(&tctx->lock);
    free(tctx);
    return NULL;
  }
  return tctx;
}

struct panelreel* panelreel_demo(WINDOW* w){
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
    .min_supported_cols = 8,
    .min_supported_rows = 5,
    .borderpair = (COLORS * (COLOR_MAGENTA + 1)) + 1,
    .borderattr = A_NORMAL,
    .tabletattr = A_NORMAL,
    .tabletpair = (COLORS * (COLOR_GREEN + 1)) + 1,
    .focusedattr = A_NORMAL,
    .focusedpair = (COLORS * (COLOR_CYAN + 1)) + 1,
  };
  tabletctx* tctxs = NULL;
  int x = 4, y = 4;
  struct panelreel* pr = create_panelreel(w, &popts, y, 0, 0, x);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return NULL;
  }
  // Press a for a new panel above the current, c for a new one below the
  // current, and b for a new block at arbitrary placement. q quits.
  int pair = COLOR_CYAN;
  wattr_set(w, A_NORMAL, 0, &pair);
  int key;
  mvwprintw(w, 1, 1, "a, b, c create tablets, q quits.");
  clrtoeol();
  do{
    pair = COLOR_RED;
    wattr_set(w, A_NORMAL, 0, &pair);
    mvwprintw(w, 2, 2, "%d tablets", panelreel_tabletcount(pr));
    pair = COLOR_BLUE;
    wattr_set(w, A_NORMAL, 0, &pair);
    key = mvwgetch(w, 3, 2);
    clrtoeol();
    struct tabletctx* newtablet = NULL;
    switch(key){
      case 'a': newtablet = new_tabletctx(pr); break;
      case 'b': newtablet = new_tabletctx(pr); break;
      case 'c': newtablet = new_tabletctx(pr); break;
      case KEY_LEFT:
      case 'h': --x; if(panelreel_move(pr, x, y)){ ++x; } break;
      case KEY_RIGHT:
      case 'l': ++x; if(panelreel_move(pr, x, y)){ --x; } break;
      case KEY_DC: del_active_tablet(pr); break;
      case 'q': break;
      default: wprintw(w, "Unknown key: %c (%d)\n", key, key);
    }
    if(newtablet){
      newtablet->next = tctxs;
      tctxs = newtablet;
    }
  }while(key != 'q');
  // FIXME join all threads
  return pr;
}
