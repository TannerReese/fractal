#ifndef _FRACTAL_H
#define _FRACTAL_H

#include <complex.h>
#include <stdbool.h>

typedef struct{
	// Transformation to apply to number before taking power
	// If trans == NULL then the value is left unchanged
	complex (*trans)(complex);
	
	// Power to raise transformed number to
	complex power;
	// Shift to add to power
	complex param;
	// Radius to check for escape
	double radius;
	
	// Full Rule: z_(n+1) = trans(z_n)^power + param
	// Test for Escape: |z_n| >= radius
} fractal_t;

// Takes absolute value of each component of a complex number
// crect(a + bi) = |a| + |b|i
complex crect(complex pt);

/* Applies given fractal rule to a complex point
 * Returns whether the new value escapes
 * 
 * Usage:
 *   fractal_t fr = {conj, 2, 0.1 + 0.5 * I, 2}; // z_(n+1) = z_n^2 + 0.1 + 0.5i
 *   complex pt = 1 - 0.3 * I;
 *   bool is_esc = frc_apply(fr, &pt);
 *   // pt == (1 + 0.3 * I)^2 + 0.1 + 0.5i == 1.01 + 1.1i
 *   // is_esc == (cabs(1.01 + 1.1i) >= 2) = (1.49335 >= 2) = false
 * 
 * Arguments:
 *   fractal_t fr : parameters indicating how the point should be moved
 *   complex *pt : pointer to complex point to take as input
 * 
 * Returns:
 *   bool : boolean indicating if the new value of pt has escaped
 *   complex *pt : the resulting complex point
 */ 
bool frc_apply(fractal_t fr, complex *pt);

/* Iteratively applies fractal rule to the point
 * 
 * Usage:
 * 
 * Arguments:
 *   fractal_t fr : parameters indicating how the point should be moved and the bounding radius
 *   complex *pt : initial point for calculating orbit
 *   int max : maximum number of iterations to perform
 *   complex *orb : pointer to array to store orbit points in
 *   int orbcap : maximum number of entries to store in orb
 * 
 * Returns:
 *   int : number of iterations performed before escaping
 *      OR -1 if point did not escape
 *   complex *pt : final value in orbit after escaping or reaching the maximum iterations
 *   complex *orb : array into which to store the orbit values
 *      NOTE points are only stored upto orbcap and not beyond
 */
int frc_orbit(fractal_t fr, complex *pt, int max, complex *orb, int orbcap);



// Identifies a rectangle within the complex plane
// Also records the number of rows and columns when converting to a grid
typedef struct{
	// Upper left-hand corner of viewing rectangle
	complex corner;
	// Size of rectangular viewing area in complex plane
	double width, height;
	
	// Number of Rows and Columns of Pixels
	int rows, columns;
} viewport_t;

// Calculate the row and column of a complex number using the given viewport
bool comp_to_rc(viewport_t vw, complex pt, int *r, int *c);
// Calculate the complex number at a given row and column using the given viewport
complex comp_at_rc(viewport_t vw, int r, int c);

#endif
