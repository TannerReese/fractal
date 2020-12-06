#include <ncurses.h>
#include <complex.h>
#include <math.h>
#include <argp.h>
#include <stdlib.h>

#include <png.h>

#include "buddha.h"


// Number of points to plot every second
float plots_per_sec = 10000;

// Rectangle in complex plane to draw to the terminal
viewport_t view = {-2 + 2 * I /* Corner */, 4 /* Width */, 4 /* Height */, 0 /* Rows */, 0 /* Columns */};

// Factor to correct bin value by to achieve appropriate brightness
// Value = (BinCount / MaxBinCount)^gamm ; 0 <= Value <= 1
double gamm = 0.5;

// Fractal parameters used to generate orbits
fractal_t rule = {NULL /* Transform */, 2 /* Power */, 0 /* Param */, 2 /* Radius */};  // Fractal rule used for generating orbits
int max_iters = 100, min_iters = 10;  // Maximum number of iterations to calculate in orbit and Minimum to accept

// Create plot to count the number of points from each orbit that fall in each bin
// Grid not allocated until runtime
plot_t plot = {{-2 + 2 * I /* Corner */, 4 /* Width */, 4 /* Height */, 1000 /* Rows */, 1000 /* Columns */}, NULL /* Grid */};
int plotted = 0;  // Tracks total number of points plotted on plot

#define SCREENSHOT_NAME_LENGTH 64
char screenshot_filename[SCREENSHOT_NAME_LENGTH] = "fractal_screenshot.png";

error_t parse_opt(int key, char *arg, struct argp_state *state){
	double real, imag;
	switch(key){
		// Provide maximum number of iterations
		case 'n':
			if(sscanf(arg, " %i", &max_iters) < 1){
				printf("Invalid input for maximum iterations, must be an integer: \"%s\"", arg);
				argp_usage(state);
			}
		break;
		// Provide minimum length of orbit that will be used
		case 'm':
			if(sscanf(arg, " %i", &min_iters) < 1){
				printf("Invalid input for minimum iterations, must be an integer: \"%s\"", arg);
				argp_usage(state);
			}
		break;
		
		// Set Transform
		case 'M': rule.trans = NULL;  // Mandelbrot:  z_(n+1) = z_n ^ p + c
		break;
		case 'B': rule.trans = crect;  // Burning Ship: z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c
		break;
		case 'T': rule.trans = conj;  // Tricorn: z_(n+1) = conj(z_n) ^ p + c
		break;
		
		// Set Power
		case 'p':
			switch(sscanf(arg, " %lf,%lf", &real, &imag)){
				case 0:
					printf("Invalid power, must be either REAL | REAL,IMAG : \"%s\"", arg);
					argp_usage(state);
				case 1: imag = 0;
				case 2:
					rule.power = real + imag * I;
			}
		break;
		// Set Bailout Radius
		case 'r':
			if(sscanf(arg, " %lf", &rule.radius) < 1){
				printf("Invalid bailout radius, must be floating point: \"%s\"", arg);
				argp_usage(state);
			}
		break;
		
		// Set center of window in complex plane
		case 'z':
			switch(sscanf(arg, " %lf,%lf", &real, &imag)){
				case 0:
					printf("Invalid window location, must be complex REAL | REAL,IMAG : \"%s\"\n", arg);
					argp_usage(state);
				case 1: imag = 0;
				case 2:
					view.corner = (real - view.width / 2) + (imag + view.height / 2) * I;
			}
		break;
		// Provide dimensions of window in complex plane
		case 'w':
			if(sscanf(arg, " %lf,%lf", &view.width, &view.height) < 2){
				printf("Invalid window dimensions, must be INT,INT : \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		// Set gamm value
		case 'g':
			if(sscanf(arg, " %lf", &gamm) < 1){
				printf("Invalid gamm, must be floating point: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		
		case 's': // Set screenshot filename
			strcpy(screenshot_filename, arg);
		break;
		case 'd': // Set dimensions of plot
			if(sscanf(arg, " %i,%i", &plot.area.columns, &plot.area.rows) < 2){
				printf("Invalid plot dimensions, should be COLUMNS,ROWS: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

struct argp_option options[] = {
	{"max_iters", 'n', "ITERATIONS", 0, "Number of iterations to perform before falling through  (default: 100)", 1},
	{"min_iters", 'm', "ITERATIONS", 0, "Minimum length of orbits to allow to be drawn to plot  (default: 10)", 1},
	{"power", 'p', "REAL[,IMAG]", 0, "Power to raise z to in iteration i.e. z_(n+1) = f(z_n) ^ p + c  (default: 2)", 1},
	{"radius", 'r', "RADIUS", 0, "Radius within which iterations will continue i.e. |z_n| < RADIUS implies z_(n+1) will be calculated (default: 2)", 1},
	{"mandel", 'M', 0, 0, "Use the standard mandelbrot rule for generation i.e. z_(n+1) = z_n ^ p + c (Standard)", 2},
	{"burning-ship", 'B', 0, 0, "Use the burning ship rule for generation i.e. z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c", 2},
	{"tricorn", 'T', 0, 0, "Use the tricorn rule for generation i.e. z_(n+1) = conj(z_n) ^ p + c", 2},
	{"position", 'z', "REAL[,IMAG]", 0, "Specify center of window when first starting  (default: 0 + 0i)", 3},
	{"window", 'w', "WIDTH,HEIGHT", 0, "Provide width and height (in complex plane, floating-point) of window  (default: 2, 2)", 3},
	{"gamma", 'g', "GAMMA", 0, "Power to raise normalized bin count to in order to obtain greyscale  (default: 0.5)", 3},
	{"screenshot", 's', "FILE", 0, "File Path to store screenshots in (default: fractal_screenshot.png)", 4},
	{"dimensions", 'd', "ROWS,HEIGHT", 0, "Provide number of rows and columns in plot  (default: 1000, 1000)", 4},
	{0}
};

struct argp argp = {options, parse_opt,
	"",
	"Display and Navigate the Buddhabrot\v"
	"Controls:\n"
		"\tArrows / WASD / HJKL -- Move viewport around complex plane\n"
		"\t'<' / '>' -- Zoom Out / Zoom In\n"
		"\t'[' / ']' -- Decrease Max Iterations / Increase Max Iterations\n"
		"\t';' / '\"' -- Decrease Min Iterations / Increase Min Iterations\n"
		"\t'-' / '+' -- Decrease Brightness / Increase Brightness\n"
		"\tC -- Clear Plot\n"
		"\tB -- Clear and Redefine Plot Area as current window\n"
		"\tP -- Pause / Play generation and plotting of orbits\n"
		"\tY -- Take Screenshot (stored to -s option)\n"
		"\tQ -- Quit\n\n"
		"Holding any Non-Assigned Key (e.g. Space Bar) speeds up generation and plotting of Orbits\n"
};



// Draw Information about Parameters to terminal
void draw_labels(complex mouse_loc);

// Takes value in the range [0, 28]
// Clamped if outside range
chtype degree_to_char(int value);

// Draw values from plot to ncurses window
void draw_plot(plot_t pl, viewport_t view, double gamm);
// Save screenshot of plot to file only showing area in vw
bool write_plot(const char *filename, plot_t pl, viewport_t vw, double gamm);

int main(int argc, char *argv[]){
	argp_parse(&argp, argc, argv, 0, NULL, NULL);
	
	// Ncurses Init
	initscr();
	curs_set(0);
	keypad(stdscr, TRUE);
	timeout(1000);
	
	// Setup Colors (According to Blackbody Radiation due to Heating)
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_YELLOW);
	init_pair(4, COLOR_CYAN, COLOR_WHITE);
	init_pair(5, COLOR_BLACK, COLOR_CYAN);
	
	// Configure plot area and Allocate grid
	view.rows = plot.area.rows;
	view.columns = plot.area.columns;
	plot.area = view;
	plot.grid = malloc(sizeof(unsigned int) * plot.area.rows * plot.area.columns);
	
	// Define area from which to draw points randomly to generate orbits
	viewport_t farm = {-2 + 2*I, 4, 4, 0, 0};
	
	// Accept mouse events
	mousemask(ALL_MOUSE_EVENTS, NULL);
	MEVENT evt;
	complex mouse_loc = 0;
	
	int c;
	bool running = 1, generating = 1;
	while(running){
		// Generate and plot new orbits
		if(generating) plotted += plot_rand(plot, farm, rule, min_iters, max_iters, (int)plots_per_sec);
		
		// Draw Plot
		draw_plot(plot, view, gamm);
		draw_labels(mouse_loc);
		refresh();
		
		// Get Keyboard command
		c = getch();
		switch(c){
			case 'w': case 'W':
			case 'k': case 'K':
			case KEY_UP:  // Move Up
				view.corner += view.height * I / 10;
			break;
			case 's': case 'S':
			case 'j': case 'J':
			case KEY_DOWN:  // Move Down
				view.corner -= view.height * I / 10;
			break;
			case 'a': case 'A':
			case 'h': case 'H':
			case KEY_LEFT:  // Move Left
				view.corner -= view.width / 10;
			break;
			case 'd': case 'D':
			case 'l': case 'L':
			case KEY_RIGHT:  // Move Right
				view.corner += view.width / 10;
			break;
			case ',': case '<':  // Zoom Out
				view.corner += -view.width * 0.05 + view.height * 0.05 * I;
				view.width *= 1.1;
				view.height *= 1.1;
			break;
			case '.': case '>':  // Zoom In
				view.corner += view.width * 0.05 - view.height * 0.05 * I;
				view.width *= 0.9;
				view.height *= 0.9;
			break;
			
			// Decrease minimum orbit length threshold
			case ';': case ':':
				min_iters -= 10;
				if(min_iters < -1) min_iters = -10;
			break;
			// Increase minimum orbit length threshold
			case '\'': case '"':
				min_iters += 10;
				if(min_iters > max_iters) min_iters = max_iters - 1;
			break;
			
			// Decrease number of iterations performed
			case '{': case '[':
				max_iters -= 10;
				if(max_iters < min_iters) max_iters = min_iters + 1;
			break;
			// Increase number of iterations performed
			case '}': case ']':
				max_iters += 10;
			break;
			
			// Decrease Gamma to Increase Brightness
			case '=': case '+': gamm /= 1.1;
			break;
			// Increase Gamma to Decrease Brightness
			case '-': case '_': gamm *= 1.1;
			break;
			
			// Clear Plot of all points
			case 'c': case 'C':
				plot_clear(plot);
				plotted = 0;
			break;
			// Clear and Redefine plot for current view
			case 'b': case 'B':
				plot_clear(plot);
				plotted = 0;
				
				view.rows = plot.area.rows;
				view.columns = plot.area.columns;
				plot.area = view;
			break;
			/*
			// Decrease generation speed
			case 'f': case 'F':
				plots_per_sec /= 1.1;
			break;
			// Increase generation speed
			case 'g': case 'G':
				plots_per_sec *= 1.1;
			break;
			*/
			
			// Pause/Play Generation of Orbits
			case 'p': case 'P':
				if(generating){
					timeout(-1);  // Turn indefinite blocking back on
					generating = 0;
				}else{
					timeout(1000);  // Turn blocking off
					generating = 1;
				}
			break;
			
			// Save screenshot of Plot
			case 'y': case 'Y':
				write_plot("buddha_screenshot.png", plot, view, gamm);
			break;
			
			
			// Calculate mouse position
			case KEY_MOUSE:
				if(getmouse(&evt) == OK){
					getmaxyx(stdscr, view.rows, view.columns);
					mouse_loc = comp_at_rc(view, evt.y, evt.x);
				}
			break;
			
			
			// Quit program
			case 'q': case 'Q': running = 0;
			break;
		}
	}	
	
	// End Ncurses
	endwin();
	
	return 0;
}

void draw_labels(complex mouse_loc){
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	
	mvprintw(rows - 2, 0, " Min, Max Iters: %i, %i     Mouse: %lf + %lf * i     Points Plotted: %i ",
		min_iters, max_iters,
		creal(mouse_loc), cimag(mouse_loc),
		plotted
	);
	
	mvprintw(rows - 1, 0, " Plot: (%lf, %lf)     Window: (%lf, %lf)     Gamma: %lf ",
		plot.area.width, plot.area.height,
		view.width, view.height,
		gamma
	);
}



chtype degree_to_char(int value){
	static char chrseq[] = " `'\"*%#";  // Sequence of characters to use for gradation
	
	// Clamp value
	if(value < 0) value = 0;
	if(value > 28) value = 28;
	
	return (chtype)(chrseq[value % 7]) | COLOR_PAIR(value / 7 + 1);
}

void draw_plot(plot_t pl, viewport_t view, double gamm){
	int width, height;
	getmaxyx(stdscr, height, width);
	
	// Allocate space for bins and clear memory
	unsigned int bins[width * height];
	memset(bins, 0, sizeof(unsigned int) * width * height);
	
	// Calculate subsection of plot area to draw
	int minr, minc, maxr, maxc;
	minr = (int)(cimag(pl.area.corner - view.corner) * pl.area.rows / pl.area.height);
	minc = (int)(creal(view.corner - pl.area.corner) * pl.area.columns / pl.area.width);
	maxr = (int)((cimag(pl.area.corner - view.corner) + view.height) * pl.area.rows / pl.area.height);
	maxc = (int)((creal(view.corner - pl.area.corner) + view.width) * pl.area.columns / pl.area.width);
	
	// Transfer counts from plot to bins
	int x, y, r, c;
	unsigned int *val, maxval = 0;
	for(r = minr; r < maxr; r++) for(c = minc; c < maxc; c++)
	if(0 <= r && r < pl.area.rows && 0 < c && c < pl.area.columns){
		x = (c - minc) * width / (maxc - minc);
		y = (r - minr) * height / (maxr - minr);
		
		// Add values from plot to corresponding bin
		val = bins + y * width + x;
		*val += plotat(pl, r, c);
		
		// Keep track of maximum value in bins
		if(*val > maxval) maxval = *val;
	}
	
	// Draw out the bins
	double scl;
	for(y = 0; y < height; y++) for(x = 0; x < width; x++){
		scl = (double)bins[y * width + x] / maxval;
		scl = pow(scl, gamm);
		mvaddch(y, x, degree_to_char((int)(scl * 29)));
	}
}



// Take snapshot of plot at current view
// Returns true if successful ; false if error
bool write_plot(const char *filename, plot_t pl, viewport_t vw, double gamm){
	FILE *fl = fopen(filename, "wb");
	if(!fl){
		fprintf(stderr, "Could not open %s to write image\n", filename);
		fclose(fl);
		return false;
	}
	
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr){
		fprintf(stderr, "Could not allocate write struct\n");
		fclose(fl);
		return false;
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr){
		fprintf(stderr, "Could not allocate info struct\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fl);
		return false;
	}
	
	if(setjmp(png_jmpbuf(png_ptr))){
		fprintf(stderr, "Error in PNG writing\n");
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fl);
		return false;
	}
	
	// Calculate subsection of plot area to draw
	int minr, minc, maxr, maxc;
	minr = (int)(cimag(pl.area.corner - vw.corner) * pl.area.rows / pl.area.height);
	minc = (int)(creal(vw.corner - pl.area.corner) * pl.area.columns / pl.area.width);
	maxr = (int)((cimag(pl.area.corner - vw.corner) + vw.height) * pl.area.rows / pl.area.height);
	maxc = (int)((creal(vw.corner - pl.area.corner) + vw.width) * pl.area.columns / pl.area.width);
	
	// Create header for file
	png_init_io(png_ptr, fl);
	png_set_IHDR(png_ptr, info_ptr, maxc - minc /* Image Width */, maxr - minr /* Image Height */, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);
	
	png_write_info(png_ptr, info_ptr);
	png_set_IHDR(png_ptr, info_ptr, maxc - minc, maxr - minr,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);
	
	int r, c;
	
	// Get max from subsection of interest
	unsigned int val, maxval = 0;
	for(r = minr; r < maxr; r++) for(c = minc; c < maxc; c++){
		val = plotat(pl, r, c);
		if(val > maxval) maxval = val;
	}
	
	double scl;
	png_color px;
	png_bytep row = png_malloc(png_ptr, (maxc - minc) * sizeof(png_color));
	// Iterate through pixels
	for(r = minr; r < maxr; r++){
		for(c = minc; c < maxc; c++){
			scl = (double)plotat(pl, r, c) / maxval;
			scl = pow(scl, gamm);  // Scale results
			
			// Create greyscale from value
			px.red = (int)(scl * 255);
			px.green = (int)(scl * 255);
			px.blue = (int)(scl * 255);
			((png_color*)row)[c - minc] = px;
		}
		png_write_row(png_ptr, row);
	}
	png_free(png_ptr, row);
	
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fl);
	return true;
}
