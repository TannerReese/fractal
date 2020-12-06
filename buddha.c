#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buddha.h"


plot_t plot_init(complex center, double width, double height, int rows, int cols){
	size_t sz = sizeof(unsigned int) * rows * cols;
	unsigned int *grid = malloc(sz);
	memset(grid, 0, sz);
	
	plot_t pl = {
		{
			center - width / 2 + (height / 2) * I,
			width, height,
			rows, cols
		},
		grid
	};
	return pl;
}

void plot_clear(plot_t pl){
	memset(pl.grid, 0, sizeof(unsigned int) * pl.area.rows * pl.area.columns);
}

void plot_free(plot_t pl){
	free(pl.grid);
}



unsigned int plot_max(plot_t pl){
	int r, c;
	unsigned int tmp, max = 0;
	for(r = 0; r < pl.area.rows; r++) for(c = 0; c < pl.area.columns; c++){
		tmp = plotat(pl, r, c);
		max = tmp > max ? tmp : max;
	}
	
	return max;
}

unsigned int *plot_atcmp(plot_t pl, complex pt){
	int r, c;
	if(comp_to_rc(pl.area, pt, &r, &c)) return &plotat(pl, r, c);
	else return NULL;
}



complex view_gener(viewport_t vw){
	return vw.corner
		+ ((double)rand() * vw.width / RAND_MAX)
		- ((double)rand() * vw.height * I / RAND_MAX)
	;
}

int plot_rand(plot_t pl, viewport_t farm, fractal_t rule, int min, int max, int numpts){
	complex pt, zero, orb[max];
	int r, c, i;
	unsigned int *bin, count = 0;
	
	srand((unsigned int)time(NULL) + (unsigned int)clock());
	
	for(; numpts > 0; numpts--){
		pt = view_gener(farm);
		
		rule.param = pt;
		zero = pt;
		i = frc_orbit(rule, &zero, max, orb, max);
		
		if(i > min){
			for(i--; i >= 0; i--){
				if(bin = plot_atcmp(pl, orb[i])){
					(*bin)++;
					count++;
				}
			}
		}
	}
	
	return count;
}

