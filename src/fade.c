#include <unistd.h>
#include <sys/time.h>
#include <outcurses.h>

int fade(unsigned sec){
	int origr[COLORS], origg[COLORS], origb[COLORS];
	short r[COLORS], g[COLORS], b[COLORS];
	// ncurses palettes are in terms of 0..1000, so there's no point in
	// trying to do more than 1000 iterations, ever. This is in usec.
	const long unsigned quanta = sec * 1000000 / 15;
	long unsigned sus, cus;
	struct timeval stime;
	int p;

	for(p = 0 ; p < COLORS ; ++p){
		if(extended_color_content(p, origr + p, origg + p, origb + p) != OK){
			return -1;
		}
		r[p] = origr[p];
		g[p] = origg[p];
		b[p] = origb[p];
	}
	gettimeofday(&stime, NULL);
	cus = sus = stime.tv_sec * 1000000 + stime.tv_usec;
	while(cus < sus + sec * 1000000){
		long unsigned permille;
		struct timeval ctime;

		if((permille = (cus - sus) * 1000 / (sec * 1000000)) > 1000){
			permille = 1000;
		}
		for(p = 0 ; p < COLORS ; ++p){
			r[p] = (origr[p] * (1000 - permille)) / 1000;
			g[p] = (origg[p] * (1000 - permille)) / 1000;
			b[p] = (origb[p] * (1000 - permille)) / 1000;
			init_extended_color(p, r[p], g[p], b[p]);
		}
		usleep(quanta);
		wrefresh(curscr);
		gettimeofday(&ctime, NULL);
		cus = ctime.tv_sec * 1000000 + ctime.tv_usec;
	}
	for(p = 0 ; p < COLORS ; ++p){
		if(init_extended_color(p, origr[p], origg[p], origb[p]) != OK){
			return -1;
		}
	}
	wrefresh(curscr);
	return 0; // FIXME get real result, feeling free to abort early
}
