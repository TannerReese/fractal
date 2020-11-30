#include <math.h>

#include "fractal.h"

complex crect(complex pt){
	return fabs(creal(pt)) + fabs(cimag(pt)) * I;
}

// Apply fractal rule to a point
bool frc_apply(fractal_t fr, complex *pt){
	if(fr.trans) *pt = fr.trans(*pt);
	*pt = cpow(*pt, fr.power);
	*pt += fr.param;
	
	// Indicate if the new point escapes
	return cabs(*pt) >= fr.radius;
}

int frc_orbit(fractal_t fr, complex *pt, int max, complex *orb, int orbcap){
	int iters;
	bool esc = cabs(*pt) >= fr.radius;
	for(iters = 0; !esc && iters < max; esc = frc_apply(fr, pt), iters++){
		// Store the current point into the orbit if there is room
		if(orb && iters < orbcap) orb[iters] = *pt;
	}
	
	// Return the iteration number if it escaped
	// Return -1 if it did not escape
	return esc ? iters : -1;
}




bool comp_to_rc(viewport_t vw, complex pt, int *r, int *c){
	pt -= vw.corner;  // Calculate offset between point and corner
	*c = (int)(creal(pt) * vw.columns / vw.width);
	*r = (int)(-cimag(pt) * vw.rows / vw.height);
	return 0 <= *r && *r < vw.rows && 0 <= *c && *c < vw.columns;
}

complex comp_at_rc(viewport_t vw, int r, int c){
	return vw.corner + (c * vw.width / vw.columns - (r * vw.height / vw.rows) * I);
}

