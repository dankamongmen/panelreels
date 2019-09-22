#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <outcurses.h>

// A set of RGB color components
typedef struct ccomps {
	int r, g, b;
} ccomps;

// These arrays are too large to be safely placed on the stack.
static int
alloc_ccomps(int count, ccomps** orig, ccomps** cur){
	*orig = malloc(count * sizeof(ccomps));
	*cur = malloc(count * sizeof(ccomps));
	if(*orig == NULL || *cur == NULL){
		free(*orig);
		free(*cur);
		*orig = NULL;
		*cur = NULL;
		return -1;
	}
	return 0;
}

// count ought be COLORS to retrieve the entire palette. If maxes is non-NULL,
// it points to a single ccomps structure, which will be filled in with the
// maximum components found.
static int
retrieve_palette(int count, ccomps* palette, ccomps* maxes){
	ccomps maxes_store;
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
	}
	return 0;
}

static int
set_palette(int count, const ccomps* palette){
	int p;

	for(p = 0 ; p < count ; ++p){
		if(init_extended_color(p, palette[p].r, palette[p].g, palette[p].b) != OK){
			return -1;
		}
	}
	return 0;
}

// Returns the number of color pairs initialized, which will not be greater
// (but might be less) than the number of colors.
static int
init_color_pairs(int pairs, int colors){
	const int pmax = colors > pairs ? pairs : colors;
	int p;

	// Pairs start at 1.
	for(p = 1 ; p < pmax ; ++p){
		if(init_extended_pair(p, p, -1) != OK){
			return -1;
		}
	}
	return pairs;
}

#define NANOSECS_IN_SEC 1000000000ull

int fade(WINDOW* w, unsigned sec){
	uint64_t nanosecs_total;
	uint64_t nanosecs_step;
	ccomps* orig;
	ccomps maxes;
	int maxsteps;
	ccomps* cur;
	int ret;

	ret = -1;
	if(alloc_ccomps(COLORS, &orig, &cur)){
		goto done;
	}
	// Retrieve current palette, and extract component maxima. There is no
	// point in doing more loop iterations than the maximum component, since
	// the smallest action we can take is subtracting 1 from the largest value.
	if(retrieve_palette(COLORS, orig, &maxes)){
		goto done;
	}
	maxsteps = maxes.g > maxes.r ? (maxes.b > maxes.g ? maxes.b : maxes.g) :
			   (maxes.b > maxes.r ? maxes.b : maxes.r);
	if(maxsteps < 1){ // no colors in use? more likely an error
		goto done;
	}
	memcpy(cur, orig, sizeof(*cur) * COLORS);
    // FIXME should be called earlier, probably during initialization
	if(init_color_pairs(COLOR_PAIRS, COLORS) <= 0){
		goto done;
	}
	// We have this many nanoseconds to work through compmax iterations. Our
	// natural rate might be slower or faster than what's desirable, so at each
	// iteration, we (a) set the palette to the intensity corresponding to time
	// since the process started, and (b) sleep if we're early (otherwise we
	// peg a core unnecessarily).
	nanosecs_total = sec * NANOSECS_IN_SEC;
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
