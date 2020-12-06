#include <ncurses.h>
#include <complex.h>
#include <math.h>
#include <stdbool.h>

#include <png.h>

#include <argp.h>

#include "fractal.h"


// Default values for params
int iterations = 100;
bool is_julia = 0;
bool radius_set = 0;  // Track whether the radius has been set to allow change of default
fractal_t rule = {NULL /* No Transform */, 2 /* Power */, 0 /* No Param */, 2 /* Bounding Radius */};


typedef struct{
	/* color_count: Number of colors to cycle through
	 * Ex: [    blue    |   orange   |    white    ]  ; color_count = 3
	 * iters_per_cycle: Number of complex iterations to fit into each cycle of all the colors
	 * Ex: [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ]  ; iters_per_cycle = 10
	 * A good ratio is   color_count * 10 = iters_per_cycle
	 */
	int color_count, iters_per_cycle;
	
	// Indicates that the color of points should be interpolated according to how far they go after escaping
	bool is_continuous;
	
	// colors: array of 24-bit colors {red, green, blue} to cycle through
	// set_color: color for non-escaping points
	png_color *colors, set_color;
} color_scheme_t;

#define SCREENSHOT_NAME_LENGTH 64
char screenshot_filename[SCREENSHOT_NAME_LENGTH] = "fractal_screenshot.png";
int scrshot_width = 1000, scrshot_height = 1000;

// Color Schemes
#define SCHEME_COUNT 3
png_color starry_colors[] = {{0, 0, 100}, {10, 75, 150}, {252, 178, 0}, {240, 252, 121}, {255, 255, 255}},
		  firey_colors[] = {{183, 60, 0}, {224, 77, 30}, {237, 244, 26}, {118, 190, 252}, {255, 255, 255}},
		  foresty_colors[] = {{23, 109, 24}, {170, 92, 32}, {175, 132, 66}, {27, 211, 205}};
color_scheme_t schemes[] = {
	{5, 50, 0, starry_colors, {0, 0, 0}}, // starry
	{5, 35, 0, firey_colors, {0, 0, 0}}, // firey
	{4, 40, 0, foresty_colors, {0, 0, 0}} // foresty
};
const char *scheme_names[] = {"starry", "firey", "foresty"};
color_scheme_t global_scheme = {0};


viewport_t view = {
	-1 + I, // Upper Left Corner
	2, 2,   // Width & Height
	0, 0    // Rows & Columns
};


error_t parse_opt(int key, char *arg, struct argp_state *state){
	double real, imag;
	switch(key){
		case 'j': // Set julia param
			is_julia = 1;
			switch(sscanf(arg, " %lf,%lf", &real, &imag)){
				case 0:
					printf("Inavlid input for julia parameter: \"%s\"\n", arg);
					argp_usage(state);
				case 1: imag = 0;
				case 2:
					rule.param = real + imag * I;
			}
		break;
		case 'n': // Set iterations
			if(sscanf(arg, " %i", &iterations) < 1){
				printf("Invalid input for iterations, must be an integer: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		case 'p': // Set power
			switch(sscanf(arg, " %lf,%lf", &real, &imag)){
				case 0:
					printf("Invalid input for power: \"%s\"\n", arg);
					argp_usage(state);
				case 1: imag = 0;
				case 2:
					rule.power = real + imag * I;
			}
		break;
		case 'r': // Set bailout radius
			if(sscanf(arg, " %lf", &rule.radius) < 1){
				printf("Invalid input for radius: \"%s\"\n", arg);
				argp_usage(state);
			}
			radius_set = 1;
		break;
		
		// Set Transform
		case 'M': rule.trans = NULL;  // Mandelbrot:  z_(n+1) = z_n ^ p + c
		break;
		case 'B': rule.trans = crect;  // Burning Ship: z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c
		break;
		case 'T': rule.trans = conj;  // Tricorn: z_(n+1) = conj(z_n) ^ p + c
		break;
		
		case 'z': // Set location of center of window in complex plane
			switch(sscanf(arg, " %lf,%lf", &real, &imag)){
				case 0:
					printf("Invalid window location given: \"%s\"\n", arg);
					argp_usage(state);
				case 1: imag = 0;
				case 2:
					view.corner = (real - view.width / 2) + (imag + view.height / 2) * I;
			}
		break;
		case 'w': // Set window width and height in complex plane
			if(sscanf(arg, " %lf,%lf", &real, &imag) < 2){
				printf("Invalid window width and/or height given: \"%s\"\n", arg);
				argp_usage(state);
			}
		
			// Shift corner to compensate for dimension change to keep center stationary
			view.corner = (creal(view.corner) + (view.width - real) / 2) + (cimag(view.corner) + (-view.height + imag) / 2) * I;
			view.width = real;
			view.height = imag;
		break;
		
		case 's': // Set screenshot filename
			strcpy(screenshot_filename, arg);
		break;
		case 'd': // Set dimensions of screenshot image
			if(sscanf(arg, " %i,%i", &scrshot_width, &scrshot_height) < 2){
				printf("Invalid screenshot dimensions: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		case 'c': // Set do continuous coloring
			global_scheme.is_continuous = 1;
			if(!radius_set) rule.radius = 100;
		break;
		case 'm':
			// Find and set scheme
			for(int i = 0; i < SCHEME_COUNT; i++){
				if(strcmp(scheme_names[i], arg) == 0){
					bool is_cont = global_scheme.is_continuous;
					global_scheme = schemes[i];
					global_scheme.is_continuous = is_cont;
					return 0;
				}
			}
			
			printf("No color scheme found called \"%s\"\n", arg);
		break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

struct argp_option options[] = {
	{"julia", 'j', "REAL[,IMAG]", 0, "Display the Julia Set for the given Complex Number (Note: if not specified mandelbrot is displayed)", 1},
	{"iter", 'n', "ITERATIONS", 0, "Number of iterations to perform before falling through  (default: 100)", 1},
	{"power", 'p', "REAL[,IMAG]", 0, "Power to raise z to in iteration i.e. z_(n+1) = f(z_n) ^ p + c  (default: 2)", 1},
	{"radius", 'r', "RADIUS", 0, "Radius within which iterations will continue i.e. |z_n| < RADIUS implies z_(n+1) will be calculated (default: 2)", 1},
	{"mandel", 'M', 0, 0, "Use the standard mandelbrot rule for generation i.e. z_(n+1) = z_n ^ p + c (Standard)", 2},
	{"burning-ship", 'B', 0, 0, "Use the burning ship rule for generation i.e. z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c", 2},
	{"tricorn", 'T', 0, 0, "Use the tricorn rule for generation i.e. z_(n+1) = conj(z_n) ^ p + c", 2},
	{"position", 'z', "REAL[,IMAG]", 0, "Specify center of window when first starting  (default: 0 + 0i)", 3},
	{"window", 'w', "WIDTH,HEIGHT", 0, "Provide width and height (in complex plane) of window  (default: 2, 2)", 3},
	{"screenshot", 's', "FILE", 0, "File Path to store screenshots in (default: fractal_screenshot.png)", 4},
	{"dimensions", 'd', "WIDTH,HEIGHT", 0, "Provide width and height (in pixels) of a screenshotted image  (default: 1000, 1000)", 4},
	{"continuous", 'c', 0, 0, "In saved screenshots, interpolate the color of points depending on how far they escape. Also sets the default radius to 100 (default: false)", 4},
	{"scheme", 'm', "SCHEME_NAME", 0, "Name of scheme (see below for provided color schemes)", 4},
	{0}
};
struct argp argp = {options, parse_opt,
	"",
	"Display and Navigate the Mandelbrot and Julia Sets\v"
	"Schemes:\n"
		"\tstarry -- blue background with orange and white highlight\n"
		"\tfirey -- dark red background with yellow and blue highlight\n"
		"\tforesty -- dark green background with cyan and yellow highlight\n"
	"\n"
	"Controls:\n"
		"\tArrows / WASD / HJKL -- Move viewport around complex plane\n"
		"\t'<' / '>' -- Zoom Out / Zoom In\n"
		"\t'[' / ']' -- Decrease Iterations / Increase Iterations\n"
		"\tC -- Toggle Continuous Coloring\n"
		"\tY -- Take Screenshot (stored to -s option)\n"
		"\tQ -- Quit"
};



// Draw the fractal of the given parameters to the terminal
void draw_complex(viewport_t vw);

// Generate the color for a given number of iterations using the scheme
png_color scheme_get_color(color_scheme_t scm, double iters);
// Writes current screen to file using global fractal parameters and color scheme
bool write_fractal(const char *filename, viewport_t vw, color_scheme_t scm);

int main(int argc, char *argv[]){
	global_scheme = schemes[0];
	argp_parse(&argp, argc, argv, 0, NULL, NULL);
	
	// Init ncurses
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);
	
	// Init Color Pairs
	start_color();
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_RED);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);
	init_pair(3, COLOR_BLACK, COLOR_GREEN);
	init_pair(4, COLOR_BLACK, COLOR_CYAN);
	init_pair(5, COLOR_BLACK, COLOR_YELLOW);
	init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(7, COLOR_BLACK, COLOR_WHITE);
	
	// Accept mouse events
	mousemask(ALL_MOUSE_EVENTS, NULL);
	
	complex mouse_loc = 0;
	
	int ch;
	MEVENT evt;
	bool running = true;
	bool screenshot_finished = false, cont_toggled = false;
	while(running){
		getmaxyx(stdscr, view.rows, view.columns);
		// Draw fractal
		draw_complex(view);
		
		// Print stats to screen
		attron(COLOR_PAIR(0));
		mvprintw(view.rows - 1, 0, "Iters: %i\tMouse: %lf + %lf * i | Window: (%lf, %lf)",
			iterations,
			creal(mouse_loc), cimag(mouse_loc),
			view.width, view.height
		);
		if(is_julia) printw("\t\tJulia At: %lf + %lf * i ", creal(rule.param), cimag(rule.param));
		
		// When screenshot is saved print message indicating it to the user
		if(screenshot_finished){
			mvprintw(view.rows - 2, 0, "Screenshot saved to %s ", screenshot_filename);
			screenshot_finished = false;
		}
		
		// When user toggles continuity indicate to user
		if(cont_toggled){
			mvprintw(view.rows - 2, 0, "Continuity turned %s ", global_scheme.is_continuous ? "On" : "Off");
			cont_toggled = false;
		}
		
		attroff(COLOR_PAIR(0));
		
		ch = getch();
		switch(ch){
			case 'w': case 'W':
			case 'k': case 'K':
			case KEY_UP: // Move Up
				view.corner += view.height * I / 10;
			break;
			case 's': case 'S':
			case 'j': case 'J':
			case KEY_DOWN: // Move Down
				view.corner -= view.height * I / 10;
			break;
			case 'a': case 'A':
			case 'h': case 'H':
			case KEY_LEFT: // Move Left
				view.corner -= view.width / 10;
			break;
			case 'd': case 'D':
			case 'l': case 'L':
			case KEY_RIGHT: // Move Right
				view.corner += view.width / 10;
			break;
			case ',': case '<': // Zoom Out
				view.corner += -view.width * 0.05 + view.height * 0.05 * I;
				view.width *= 1.1;
				view.height *= 1.1;
			break;
			case '.': case '>': // Zoom In
				view.corner += view.width * 0.05 - view.height * 0.05 * I;
				view.width *= 0.9;
				view.height *= 0.9;
			break;
			
			// Decrease number of iterations performed
			case '{': case '[': iterations -= 10;
			break;
			// Increase number of iterations performed
			case '}': case ']': iterations += 10;
			break;
			
			// Take screenshot
			case 'y': case 'Y':
				view.rows = scrshot_height;
				view.columns = scrshot_width;
				write_fractal(screenshot_filename, view, global_scheme);
				screenshot_finished = true;
			break;
			
			// Toggle continuous coloring for screenshots
			case 'c': case 'C':
				global_scheme.is_continuous ^= 1;
				cont_toggled = true;
			break;
			
			// Calculate mouse position
			case KEY_MOUSE:
				if(getmouse(&evt) == OK){
					mouse_loc = comp_at_rc(view, evt.y, evt.x);
				}
			break;
			
			// Quit
			case 'q': case 'Q':
				running = false;
			break;
		}
	
		usleep(10000); // Sleep for a centisecond to relieve stress on CPU
	}
	
	endwin();
	return 0;
}




// Draw complex grid to terminal screen
void draw_complex(viewport_t vw){
	int rows, cols, i;
	complex cmp, seed = rule.param;
	
	getmaxyx(stdscr, vw.rows, vw.columns);
	for(int r = 0; r < vw.rows; r++) for(int c = 0; c < vw.columns; c++){
		cmp = comp_at_rc(vw, r, c);
		
		// Create orbit and count length
		if(!is_julia){
			rule.param = cmp;
			cmp = seed;
		}
		i = frc_orbit(rule, &cmp, iterations, NULL, 0);
		
		// Restrict colors to 7 (displayable by terminal)
		i = i < 0 ? 0 : i % 7 + 1;
		
		// Draw space character
		attron(COLOR_PAIR(i));
		mvaddch(r, c, ' ');
		attroff(COLOR_PAIR(i));
	}
	
	rule.param = seed;
}



png_color scheme_get_color(color_scheme_t scm, double iters){
	// If point is in set return set_color
	if(iters < 0) return scm.set_color;
	
	// Collapse iters into the cycle length using modulo
	iters = fmod(iters, (double)scm.iters_per_cycle);
	int n = (int)(iters * scm.color_count / scm.iters_per_cycle);
	
	// Find 'distance' between iters and prior and subsequent colors
	double r, s;
	r = iters * scm.color_count / scm.iters_per_cycle - n;
	s = 1 - r;
	
	png_color c1 = scm.colors[n], c2 = scm.colors[(n + 1) % scm.color_count];
	
	// Weighted average of colors
	c1.red = (int)(s * c1.red + r * c2.red);
	c1.green = (int)(s * c1.green + r * c2.green);
	c1.blue = (int)(s * c1.blue + r * c2.blue);
	return c1;
}

// Take snapshot of set at current location
// Returns true if successful ; false if error
bool write_fractal(const char *filename, viewport_t vw, color_scheme_t scm){
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
	
	png_init_io(png_ptr, fl);
	png_set_IHDR(png_ptr, info_ptr, view.columns, view.rows, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);
	
	png_write_info(png_ptr, info_ptr);
	png_set_IHDR(png_ptr, info_ptr, view.columns, view.rows,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);
	
	png_bytep row;
	int r, c;
	double i;
	complex cmp, seed = rule.param;
	// Iterate through pixels
	row = png_malloc(png_ptr, view.columns * sizeof(png_color));
	for(r = 0; r < view.rows; r++){
		for(c = 0; c < view.columns; c++){
			cmp = comp_at_rc(vw, r, c);
			
			// Calculate length of orbit before escape
			if(!is_julia){
				rule.param = cmp;
				cmp = seed;
			}
			i = frc_orbit(rule, &cmp, iterations, NULL, 0);
			
			if(scm.is_continuous && i > 0){
				i -= log(log(cabs(cmp)) / log(rule.radius)) / log(cabs(rule.power));
			}
			
			((png_color*)row)[c] = scheme_get_color(scm, i);
		}
		png_write_row(png_ptr, row);
	}
	png_free(png_ptr, row);
	
	rule.param = seed;
	
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fl);
	return true;
}
