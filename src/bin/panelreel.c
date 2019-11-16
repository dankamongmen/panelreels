#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <outcurses.h>
#include "demo.h"

typedef struct tabletctx {
  pthread_t tid;
  struct panelreel* pr;
  struct tablet* t;
  int lines;
  int cpair;
  unsigned id;
  struct tabletctx* next;
  pthread_mutex_t lock;
} tabletctx;

static void
kill_tablet(tabletctx** tctx){
  tabletctx* t = *tctx;
  if(pthread_cancel(t->tid)){
    fprintf(stderr, "Warning: error sending pthread_cancel (%s)\n", strerror(errno));
  }
  if(pthread_join(t->tid, NULL)){
    fprintf(stderr, "Warning: error joining pthread (%s)\n", strerror(errno));
  }
  del_tablet(t->pr, t->t);
  *tctx = t->next;
  pthread_mutex_destroy(&t->lock);
  free(t);
}

static int
tabletdraw(PANEL* p, int begx, int begy, int maxx, int maxy, bool cliptop,
           void* vtabletctx){
  int err = OK;
  tabletctx* tctx = vtabletctx;
  pthread_mutex_lock(&tctx->lock);
  int x, y;
  WINDOW* w = panel_window(p);
  cchar_t cch;
  wchar_t cchbuf[2];
  swprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), L"%x", tctx->lines % 16);
  setcchar(&cch, cchbuf, A_NORMAL, 0, &tctx->cpair);
  for(y = begy ; y <= maxy ; ++y){
    if(y - begy >= tctx->lines){
      break;
    }
    err |= wmove(w, y, begx);
    for(x = begx ; x <= maxx ; ++x){
      // lower-right corner always returns an error unless scrollok() is used
      /*err |= */wadd_wch(w, &cch);
    }
  }
  int cpair = COLOR_BRIGHTWHITE;
  wattr_set(w, A_NORMAL, 0, &cpair);
  setcchar(&cch, cchbuf, A_NORMAL, 0, &cpair);
  if(cliptop){
    err |= mvwprintw(w, maxy, begx, "[#%u %dll %u/%u] ", tctx->id, tctx->lines, begy, maxy);
  }else{
    err |= mvwprintw(w, begy, begx, "[#%u %dll %u/%u] ", tctx->id, tctx->lines, begy, maxy);
  }
fprintf(stderr, "  \\--> callback for %d, %d lines (%d/%d -> %d/%d) wrote: %d ret: %d\n", tctx->id,
    tctx->lines, begy, begx, maxy, maxx, y - begy, err);
  pthread_mutex_unlock(&tctx->lock);
  assert(OK == err);
  return y - begy;
}

// Each tablet has an associated thread which will periodically send update
// events for its tablet.
static void*
tablet_thread(void* vtabletctx){
  tabletctx* tctx = vtabletctx;
  while(true){
    struct timespec ts;
    ts.tv_sec = random() % 3;
    ts.tv_nsec = random() % 1000000000;
    nanosleep(&ts, NULL);
    int action = random() % 5;
    if(action < 2){
      if((tctx->lines -= (action + 1)) < 1){
        tctx->lines = 1;
      }
      tablet_update(tctx->pr, tctx->t);
    }else if(action > 2){
      if((tctx->lines += (action - 2)) < 1){
        tctx->lines = 1;
      }
      tablet_update(tctx->pr, tctx->t);
    }
  }
  return tctx;
}

static tabletctx*
new_tabletctx(struct panelreel* pr, unsigned *id){
  tabletctx* tctx = malloc(sizeof(*tctx));
  if(tctx == NULL){
    return NULL;
  }
  pthread_mutex_init(&tctx->lock, NULL);
  tctx->pr = pr;
  tctx->lines = random() % 10 + 1; // FIXME a nice gaussian would be swell
  tctx->cpair = random() % COLORS;
  tctx->id = ++*id;
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
  int x = 4, y = 4;
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
    .min_supported_cols = 8,
    .min_supported_rows = 5,
    .borderpair = COLOR_MAGENTA,
    .borderattr = A_NORMAL,
    .tabletattr = A_NORMAL,
    .tabletpair = COLOR_GREEN,
    .focusedattr = A_NORMAL,
    .focusedpair = (COLORS * (COLOR_CYAN + 1)) + 1,
    .toff = y,
    .loff = x,
    .roff = 0,
    .boff = 0,
  };
  tabletctx* tctxs = NULL;
  struct panelreel* pr = create_panelreel(w, &popts);
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
  unsigned id = 0;
  do{
    pair = COLOR_RED;
    wattr_set(w, A_NORMAL, 0, &pair);
    int count = panelreel_tabletcount(pr);
    mvwprintw(w, 2, 2, "%d tablet%s", count, count == 1 ? "" : "s");
    wclrtoeol(w);
    pair = COLOR_BLUE;
    wattr_set(w, A_NORMAL, 0, &pair);
    key = mvwgetch(w, 3, 2);
    clrtoeol();
    struct tabletctx* newtablet = NULL;
    clrtoeol();
    switch(key){
      case 'a': newtablet = new_tabletctx(pr, &id); break;
      case 'b': newtablet = new_tabletctx(pr, &id); break;
      case 'c': newtablet = new_tabletctx(pr, &id); break;
      case KEY_LEFT:
      case 'h': --x; if(panelreel_move(pr, x, y)){ ++x; } break;
      case KEY_RIGHT:
      case 'l': ++x; if(panelreel_move(pr, x, y)){ --x; } break;
      case KEY_UP:
      case 'k': panelreel_prev(pr); break;
      case KEY_DOWN:
      case 'j': panelreel_next(pr); break;
      case KEY_DC: kill_tablet(&tctxs); break;
      case 'q': break;
      default: mvwprintw(w, 3, 2, "Unknown keycode (%d)\n", key);
    }
    if(newtablet){
      newtablet->next = tctxs;
      tctxs = newtablet;
    }
    panelreel_validate(w, pr); // do what, if not assert()ing? FIXME
  }while(key != 'q');
  while(tctxs){
    kill_tablet(&tctxs);
  }
  return pr;
}
