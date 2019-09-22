#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

// count ought be COLORS to retrieve the entire palette.
static int
retrieve_palette(int count, ccomps* palette){
	int p;

	for(p = 0 ; p < count ; ++p){
		if(extended_color_content(p, &palette[p].r, &palette[p].g, &palette[p].b) != OK){
			return -1;
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

int fade(WINDOW* w, unsigned sec){
	ccomps* orig;
	ccomps* cur;
	int ret;

	ret = -1;
	if(alloc_ccomps(COLORS, &orig, &cur)){
		goto done;
	}
	// ncurses palettes are in terms of 0..1000, so there's no point in
	// trying to do more than 1000 iterations, ever. This is in usec.
	const long unsigned quanta = sec * 1000000 / 15;
	long unsigned sus, cus;
	struct timeval stime;

	if(retrieve_palette(COLORS, orig)){
			goto done;
	}
	memcpy(cur, orig, sizeof(*cur) * COLORS);
	gettimeofday(&stime, NULL);
	cus = sus = stime.tv_sec * 1000000 + stime.tv_usec;
	while(cus < sus + sec * 1000000){
		const int pairs = COLORS > COLOR_PAIRS ? COLOR_PAIRS : COLORS;
		long unsigned permille;
		struct timeval ctime;
		int p;

		if((permille = (cus - sus) * 1000 / (sec * 1000000)) > 1000){
			permille = 1000;
		}
		for(p = 0 ; p < COLORS ; ++p){
			cur[p].r = (orig[p].r * (1000 - permille)) / 1000;
			cur[p].g = (orig[p].g * (1000 - permille)) / 1000;
			cur[p].b = (orig[p].b * (1000 - permille)) / 1000;
			if(init_extended_color(p, cur[p].r, cur[p].g, cur[p].b) != OK){
				goto done;
			}
		}
		usleep(quanta);
		for(p = 0 ; p < pairs ; ++p){
			if(init_extended_pair(p, p, -1) != OK){
				goto done;
			}
		}
	    wrefresh(w);
		gettimeofday(&ctime, NULL);
		cus = ctime.tv_sec * 1000000 + ctime.tv_usec;
	}
	if(set_palette(COLORS, orig)){
		goto done;
	}
	wrefresh(w);
	ret = 0;

done:
	free(orig);
	free(cur);
	return ret;
}
