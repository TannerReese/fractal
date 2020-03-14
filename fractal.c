#include <ncurses.h>
#include <complex.h>
#include <math.h>
#include <stdbool.h>

#include <png.h>

#include <argp.h>


enum fractal_type{
	SIMPLE_POWER, // Mandelbrot / Julia Set
	ABSOLUTE_POWER, // Burning Ship
	CONJUGATE_POWER // Tricorn
};

// Default values for params
struct fractal_params{
	int iterations, power;
	enum fractal_type type;
	bool is_julia;
	complex julia_param;
} global_params = {100, 2, false, 0};


typedef struct{
	/* color_count: Number of colors to cycle through
	 * Ex: [    blue    |   orange   |    white    ]  ; color_count = 3
	 * iters_per_cycle: Number of complex iterations to fit into each cycle of all the colors
	 * Ex: [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ]  ; iters_per_cycle = 10
	 * A good ratio is   color_count * 10 = iters_per_cycle
	 */
	int color_count, iters_per_cycle;
	
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
	{5, 50, starry_colors, {0, 0, 0}}, // starry
	{5, 35, firey_colors, {0, 0, 0}}, // firey
	{4, 40, foresty_colors, {0, 0, 0}} // foresty
};
const char *scheme_names[] = {"starry", "firey", "foresty"};
color_scheme_t global_scheme = {0};


typedef struct{
	complex corner;
	double width, height;
} viewport_t;

viewport_t view = {-1 + I, 2, 2};


error_t parse_opt(int key, char *arg, struct argp_state *state){
	double real, imag;
	switch(key){
		case 'j': // Set julia param
			global_params.is_julia = true;
			sscanf(arg, " %lf,%lf", &real, &imag);
			global_params.julia_param = real + imag * I;
		break;
		case 'n': // Set iterations
			sscanf(arg, " %i", &global_params.iterations);
		break;
		case 'p': // Set power
			sscanf(arg, " %i", &global_params.power);
		break;
		case 'M': global_params.type = SIMPLE_POWER;  // Mandelbrot:  z_(n+1) = z_n ^ p + c
		break;
		case 'B': global_params.type = ABSOLUTE_POWER;  // Burning Ship: z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c
		break;
		case 'T': global_params.type = CONJUGATE_POWER;  // Tricorn: z_(n+1) = conj(z_n) ^ p + c
		break;
		case 'z': // Set window location in complex plane
			sscanf(arg, " %lf,%lf", &real, &imag);
			view.corner = (real - view.width / 2) + (imag + view.height / 2) * I;
		break;
		case 'w': // Set window width and height in complex plane
			sscanf(arg, " %lf,%lf", &real, &imag);
			// Shift corner to compensate for dimension change to keep center stationary
			view.corner = (creal(view.corner) + (view.width - real) / 2) + (cimag(view.corner) + (-view.height + imag) / 2) * I;
			view.width = real;
			view.height = imag;
		break;
		
		case 's': // Set screenshot filename
			strcpy(screenshot_filename, arg);
		break;
		case 'd': // Set dimensions of screenshot image
			sscanf(arg, " %i,%i", &scrshot_width, &scrshot_height);
		break;
		case 'm':
			// Find and set scheme
			for(int i = 0; i < SCHEME_COUNT; i++){
				if(strcmp(scheme_names[i], arg) == 0){
					global_scheme = schemes[i];
					return 0;
				}
			}
			
			printf("No color scheme found called %s\n", arg);
		break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

struct argp_option options[] = {
	{"julia", 'j', "REAL,IMAG", 0, "Display the Julia Set for the given Complex Number (Note: if not specified mandelbrot is displayed)"},
	{"iter", 'n', "ITERATIONS", 0, "Number of iterations to perform before falling through  (default: 100)"},
	{"power", 'p', "POWER", 0, "Power to raise z to in iteration i.e. z_(n+1) = f(z_n) ^ p + c  (default: 2)"},
	{"mandel", 'M', 0, 0, "Use the standard mandelbrot rule for generation i.e. z_(n+1) = z_n ^ p + c (Standard)"},
	{"burning-ship", 'B', 0, 0, "Use the burning ship rule for generation i.e. z_(n+1) = (|Re{z_n}| + i * |Im{z_n}|) ^ p + c"},
	{"tricorn", 'T', 0, 0, "Use the tricorn rule for generation i.e. z_(n+1) = conj(z_n) ^ p + c"},
	{"position", 'z', "REAL,IMAG", 0, "Specify center of window when first starting  (default: 0 + 0i)"},
	{"window", 'w', "WIDTH,HEIGHT", 0, "Provide width and height (in complex plane) of window  (default: 2, 2)"},
	{"screenshot", 's', "FILE", 0, "File Path to store screenshots in (default: fractal_screenshot.png)"},
	{"dimensions", 'd', "WIDTH,HEIGHT", 0, "Provide width and height (in pixels) of a screenshotted image  (default: 1000, 1000)"},
	{"scheme", 'm', "SCHEME_NAME", 0, "Name of scheme (see below for provided color schemes)"},
	{0}
};
struct argp argp = {options, parse_opt,
	"",
	"Display and Navigate the Mandelbrot and Julia Sets\vSchemes:\n\tstarry -- blue background with orange and white highlight\n\tfirey -- dark red background with yellow and blue highlight\n\tforesty -- dark green background with cyan and yellow highlight\n\nControls:\n\tArrows / WASD / HJKL -- Move viewport around complex plane\n\t'<' / '>' -- Zoom Out / Zoom In\n\t'[' / ']' -- Decrease Iterations / Increase Iterations\n\tY -- Take Screenshot (stored to -s option)\n\tQ -- Quit"
};




complex comp_at_rc(viewport_t vw, int r, int c, int rows, int cols);
void comp_to_rc(viewport_t vw, complex cmp, int *r, int *c, int rows, int cols);
int orbit_count(enum fractal_type tp, complex point, complex param, int power, int max_iters);
void draw_complex(viewport_t vw, struct fractal_params prms);



png_color scheme_get_color(color_scheme_t scm, int iters);
bool write_fractal(const char *filename, viewport_t vw, struct fractal_params prms, int width, int height, color_scheme_t scm);

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
	int rows, cols;
	bool screenshot_finished = false;
	while(running){
		getmaxyx(stdscr, rows, cols);
		// Draw fractal
		draw_complex(view, global_params);
		
		// Print stats to screen
		attron(COLOR_PAIR(0));
		mvprintw(rows - 1, 0, "Iters: %i\tMouse: %lf + %lf * i | Window: (%lf, %lf)",
			global_params.iterations,
			creal(mouse_loc), cimag(mouse_loc),
			view.width, view.height
		);
		if(global_params.is_julia) printw("\t\tJulia At: %lf + %lf * i ", creal(global_params.julia_param), cimag(global_params.julia_param));
		if(screenshot_finished){
			mvprintw(rows - 2, 0, "Screenshot saved to %s ", screenshot_filename);
			screenshot_finished = false;
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
			case '{': case '[': global_params.iterations -= 10;
			break;
			case '}': case ']': global_params.iterations += 10;
			break;
			case 'y': case 'Y':
				write_fractal(screenshot_filename, view, global_params, scrshot_width, scrshot_height, global_scheme);
				screenshot_finished = true;
			break;
			case KEY_MOUSE:
				if(getmouse(&evt) == OK){
					mouse_loc = comp_at_rc(view, evt.y, evt.x, rows, cols);
				}
			break;
			case 'q': case 'Q':
				running = false;
			break;
		}
	
		usleep(10000); // Sleep for a centisecond to relieve stress on CPU
	}
	
	endwin();
	return 0;
}



// Calculate complex number for given row and column on screen
complex comp_at_rc(viewport_t vw, int r, int c, int rows, int cols){
	complex shift;
	shift = (double)c * vw.width / cols;
	shift += -I * (double)r * vw.height / rows;
	return vw.corner + shift;
}

// Calculate row and column for given complex number
void comp_to_rc(viewport_t vw, complex cmp, int *r, int *c, int rows, int cols){
	complex shift = cmp - vw.corner;
	*c = creal(shift) * cols / vw.width;
	*r = -cimag(cmp) * rows / vw.height;
}

int orbit_count(enum fractal_type tp, complex point, complex param, int power, int max_iters){
	int count = 0, p;
	complex prod;
	// Generate orbit of given point using z_(n+1) = z_n ^ p + param with z_0 = point
	while(cabs(point) < 2 && count < max_iters){
		// Apply transform
		switch(tp){
			case ABSOLUTE_POWER: point = fabs(creal(point)) + I * fabs(cimag(point));
			break;
			case CONJUGATE_POWER: point = conj(point);
			break;
			case SIMPLE_POWER:
			default:
			break;
		}
		
		prod = point;
		for(p = power - 1; p > 0; p--) prod *= point;
		point = prod + param;
		count++;
	}
	
	return count == max_iters ? -1 : count;
}



// Draw complex grid to terminal screen
void draw_complex(viewport_t vw, struct fractal_params prms){
	int rows, cols, i;
	complex cmp;
	
	getmaxyx(stdscr, rows, cols);
	for(int r = 0; r < rows; r++) for(int c = 0; c < cols; c++){
		cmp = comp_at_rc(vw, r, c, rows, cols);
		// Create orbit and count length
		if(prms.is_julia){
			i = orbit_count(prms.type, cmp, prms.julia_param, prms.power, prms.iterations);
		}else{
			i = orbit_count(prms.type, prms.julia_param, cmp, prms.power, prms.iterations);
		}
		
		// Restrict colors to 7 (displayable by terminal)
		i = i < 0 ? 0 : i % 7 + 1;
		
		// Draw space character
		attron(COLOR_PAIR(i));
		mvaddch(r, c, ' ');
		attroff(COLOR_PAIR(i));
	}
}



png_color scheme_get_color(color_scheme_t scm, int iters){
	// If point is in set return set_color
	if(iters < 0) return scm.set_color;
	
	iters %= scm.iters_per_cycle;
	int n = iters * scm.color_count / scm.iters_per_cycle;
	// Find 'distance' between iters and prior and subsequent colors
	int r, s;
	r = iters - n * scm.iters_per_cycle / scm.color_count;
	s = scm.iters_per_cycle / scm.color_count - r;
	
	png_color c1 = scm.colors[n], c2 = scm.colors[(n + 1) % scm.color_count];
	
	// Weighted average of colors
	c1.red = (s * c1.red + r * c2.red) * scm.color_count / scm.iters_per_cycle;
	c1.green = (s * c1.green + r * c2.green) * scm.color_count / scm.iters_per_cycle;
	c1.blue = (s * c1.blue + r * c2.blue) * scm.color_count / scm.iters_per_cycle;
	return c1;
}

// Take snapshot of set at current location
// Returns true if successful ; false if error
bool write_fractal(const char *filename, viewport_t vw, struct fractal_params prms, int width, int height, color_scheme_t scm){
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
	png_set_IHDR(png_ptr, info_ptr, width, height, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
	);
	
	png_write_info(png_ptr, info_ptr);
	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);
	
	png_bytep row;
	int r, c, i;
	complex cmp;
	for(r = 0; r < height; r++){
		row = png_malloc(png_ptr, width * sizeof(png_color));
		for(c = 0; c < width; c++){
			cmp = comp_at_rc(vw, r, c, height, width);
			if(prms.is_julia){
				i = orbit_count(prms.type, cmp, prms.julia_param, prms.power, prms.iterations);
			}else{
				i = orbit_count(prms.type, prms.julia_param, cmp, prms.power, prms.iterations);
			}
			
			((png_color*)row)[c] = scheme_get_color(scm, i);
		}
		png_write_row(png_ptr, row);
	}
	
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fl);
	return true;
}
