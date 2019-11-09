#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <outcurses.h>

// These arrays are too large to be safely placed on the stack.
static int
alloc_palette(int count, outcurses_rgb** orig, outcurses_rgb** cur){
  *orig = malloc(count * sizeof(outcurses_rgb));
  *cur = malloc(count * sizeof(outcurses_rgb));
  if(*orig == NULL || *cur == NULL){
    free(*orig);
    free(*cur);
    *orig = NULL;
    *cur = NULL;
    return -1;
  }
  return 0;
}

int retrieve_palette(int count, outcurses_rgb* palette, outcurses_rgb* maxes,
                     bool zeroout){
  outcurses_rgb maxes_store;
  int p;

  if(maxes == NULL){
    maxes = &maxes_store;
  }
  maxes->r = maxes->g = maxes->b = -1;
  for(p = 0 ; p < count ; ++p){
    if(extended_color_content(p, &palette[p].r, &palette[p].g, &palette[p].b) != OK){
      return -1;
    }
    if(palette[p].r > maxes->r){
      maxes->r = palette[p].r;
    }
    if(palette[p].g > maxes->g){
      maxes->g = palette[p].g;
    }
    if(palette[p].b > maxes->b){
      maxes->b = palette[p].b;
    }
    if(zeroout){
      if(init_extended_color(p, 0, 0, 0) != OK){
        return -1;
      }
    }
  }
  return 0;
}

int set_palette(int count, const outcurses_rgb* palette){
  int p;

  for(p = 0 ; p < count ; ++p){
    if(init_extended_color(p, palette[p].r, palette[p].g, palette[p].b) != OK){
      return -1;
    }
  }
  return 0;
}

#define NANOSECS_IN_SEC 1000000000ull
#define NANOSECS_IN_MS  (NANOSECS_IN_SEC / 1000ul)

int fadeout(WINDOW* w, unsigned ms){
  uint64_t nanosecs_total;
  uint64_t nanosecs_step;
  outcurses_rgb* orig;
  outcurses_rgb maxes;
  int maxsteps;
  outcurses_rgb* cur;
  int ret;

  ret = -1;
  if(alloc_palette(COLORS, &orig, &cur)){
    goto done;
  }
  // Retrieve current palette, and extract component maxima. There is no
  // point in doing more loop iterations than the maximum component, since
  // the smallest action we can take is subtracting 1 from the largest value.
  if(retrieve_palette(COLORS, orig, &maxes, false)){
    goto done;
  }
  maxsteps = maxes.g > maxes.r ? (maxes.b > maxes.g ? maxes.b : maxes.g) :
         (maxes.b > maxes.r ? maxes.b : maxes.r);
  if(maxsteps < 1){ // no colors in use? more likely an error
    goto done;
  }
  memcpy(cur, orig, sizeof(*cur) * COLORS);
  // We have this many nanoseconds to work through compmax iterations. Our
  // natural rate might be slower or faster than what's desirable, so at each
  // iteration, we (a) set the palette to the intensity corresponding to time
  // since the process started, and (b) sleep if we're early (otherwise we
  // peg a core unnecessarily).
  nanosecs_total = ms * NANOSECS_IN_MS;
  // Number of nanoseconds in an ideal steptime
  nanosecs_step = nanosecs_total / maxsteps;
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
  // Current time, sampled each iteration
  uint64_t curns;
  int p;
  do{
    clock_gettime(CLOCK_MONOTONIC, &times);
    curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > maxsteps){
      break;
    }
    for(p = 0 ; p < COLORS ; ++p){
      cur[p].r = orig[p].r * (maxsteps - iter) / maxsteps;
      cur[p].g = orig[p].g * (maxsteps - iter) / maxsteps;
      cur[p].b = orig[p].b * (maxsteps - iter) / maxsteps;
      if(init_extended_color(p, cur[p].r, cur[p].g, cur[p].b) != OK){
        goto done;
      }
    }
    wrefresh(w);
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    int r;
    // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
    // of Glibc 2.29 + Linux 5.3 :/.
    r = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    if(r){
      goto done;
    }
  }while(true);
  if(set_palette(COLORS, orig)){
    goto done;
  }
  reset_color_pairs();
  ret = 0;

done:
  free(orig);
  free(cur);
  return ret;
}

static int
find_palette_max(int count, const outcurses_rgb* palette){
  int max = 0;
  while(count--){
    if(palette[count].r > max){
      max = palette[count].r;
    }
    if(palette[count].g > max){
      max = palette[count].g;
    }
    if(palette[count].b > max){
      max = palette[count].b;
    }
  }
  return max;
}

int fadein(WINDOW* w, int count, const outcurses_rgb* palette, unsigned ms){
  int max = find_palette_max(count, palette);
  uint64_t nanosecs_total = ms * NANOSECS_IN_MS;
  // Number of nanoseconds in an ideal steptime
  uint64_t nanosecs_step = nanosecs_total / max;
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
  // Current time, sampled each iteration
  uint64_t curns;
  int ret = -1;
  int p;
  outcurses_rgb* cur = malloc(sizeof(*cur) * count);
  do{
    clock_gettime(CLOCK_MONOTONIC, &times);
    curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > max){
      break;
    }
    for(p = 0 ; p < COLORS ; ++p){
      cur[p].r = palette[p].r * iter / max;
      cur[p].g = palette[p].g * iter / max;
      cur[p].b = palette[p].b * iter / max;
      if(init_extended_color(p, cur[p].r, cur[p].g, cur[p].b) != OK){
        goto done;
      }
    }
    wrefresh(w);
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    int r;
    // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
    // of Glibc 2.29 + Linux 5.3 :/.
    r = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    if(r){
      goto done;
    }
  }while(true);
  ret = 0;

done:
  free(cur);
  return ret;
}
