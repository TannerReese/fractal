#ifndef _BUDDHA_H
#define _BUDDHA_H

#include "fractal.h"


// Get grid value from plot at given row and column
#define plotat(p, r, c) ((p).grid[(p).area.columns * (r) + (c)])

// Grid for counting points
typedef struct{
	// Rectangle in the complex plane that grid corresponds to
	viewport_t area;
	
	// Grid of bins counting the number of points in each
	unsigned int *grid;
} plot_t;

// Allocate memory and initialize fields for plot
plot_t plot_init(complex center, double width, double height, int rows, int cols);
// Set every count in grid to zero
void plot_clear(plot_t pl);
// Deallocate memory for plot
void plot_free(plot_t pl);

// Get maximum value in grid
unsigned int plot_max(plot_t pl);
// Get value of grid at given complex number
unsigned int *plot_atcmp(plot_t pl, complex pt);

// Generate random point from given viewport using 2D uniform distribution
complex view_gener(viewport_t vw);

// Add points from `numpts` number of orbits to `pl`
// Only include orbits with lengths between `min` and `max`
int plot_rand(plot_t pl, viewport_t farm, fractal_t rule, int min, int max, int numpts);

#endif
