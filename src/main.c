/*
 * main.c
 *
 *  Created on: Dec 23, 2019
 *      Author: Matheus Pereira Nogueira
 */


#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



#define NAV_SPEED 4
#define FPS 1000
#define MAX_SCREEN 100
#define QUIT_CHAR 'q'
#define DEPTH 8
#define INDEX(x, y, w) ((y)*(w) + (x))
#define SCALE(x, s) ((x)*(s)/100)
#define INV_SCALE(y, s) ((y)*(100)/(s))

enum {
	BOLD = 0,
	NORMAL,
	DIM,
};

enum {
	BLACK = 0,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE
};

const uint8_t charlist[] = {'&', '#', '#', 'g', 'i', 'p','=', '.'};
const uint8_t chrattrindex[] = {BOLD, BOLD, NORMAL, NORMAL, NORMAL, DIM, DIM, DIM};
const uint32_t attrlist[] = {A_BOLD, A_NORMAL, A_DIM};


typedef struct rgb_data {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_data;

typedef struct raw_image {
	rgb_data* r_data;
	const char* filepath;
	int width;
	int height;
	int channels;
} r_img;

typedef struct processed_px {
	uint8_t chr;
	uint8_t attr;
	uint8_t color;
} p_px;

typedef struct processed_img {
	p_px* p_data;
	int width;
	int height;
} p_img;

typedef struct screen {
	int color;
	int cols;
	int rows;
	int init_row;
	int init_col;
	int scaling;
	const char* i_filepath;
	p_img p_image;
	p_img sc_image;
} screen;

p_px empty_px = {' ', 2, BLACK };

r_img* image_load(const char* filepath){
	r_img* img = malloc(sizeof(r_img));
	img->filepath = filepath;
	img->r_data = (rgb_data*)stbi_load(
			img->filepath,
			&(img->width),
			&(img->height),
			&(img->channels),
			3
	);
	return img;
}

void image_free(r_img* r_image){
	stbi_image_free(r_image->r_data);
	free(r_image);
}

uint8_t color_8bit(const rgb_data pixel) {
	uint8_t color_filter = (pixel.r + pixel.g + pixel.b)/3;
	uint8_t r = 0, g = 0 , b = 0;
	uint8_t color;


	if(pixel.r > color_filter)
		r = 1;
	if(pixel.g > color_filter)
		g = 1;
	if(pixel.b > color_filter)
		b = 1;

	if(!r && !g && !b) {
		color = BLACK;
	} else if(!r && !g && b) {
		color = BLUE;
	} else if(!r && g && !b) {
		color = GREEN;
	} else if(!r && g && b) {
		color = CYAN;
	} else if(r && !g && !b) {
		color = RED;
	} else if(r && !g && b) {
		color = MAGENTA;
	} else if(r && g && !b) {
		color = YELLOW;
	} else {
		color = WHITE;
	}

	return color;
}

p_px pixel_char(const rgb_data pixel) {
	int sum = pixel.r + pixel.g + pixel.b;
	p_px p_pixel;

	for(int i=0; i < DEPTH; i++){
		if(sum >= (765/DEPTH)*i && sum < (765/DEPTH)*(1+i)){
			p_pixel.chr = charlist[DEPTH-i-1];
			p_pixel.attr = chrattrindex[DEPTH-i-1];
		}
	}
	p_pixel.color = color_8bit(pixel);
	return p_pixel;
}

p_img process_r_image(const r_img img) {
	p_img p_image;
	p_image.p_data = malloc(sizeof(p_px)*img.width*img.height);
	p_image.width = img.width;
	p_image.height = img.height;

	rgb_data raw_pixel;
	for(int i=0; i < img.width; i++){
		for(int j=0; j < img.height; j++){
			raw_pixel = img.r_data[INDEX(i, j, img.width)];
			p_image.p_data[INDEX(i, j, img.width)] = pixel_char(raw_pixel);
		}
	}
	return p_image;
}

void draw_ui(const screen s) {
	char msg[MAX_SCREEN];

	for(int i=0;i<s.cols-1;i++){
		mvaddch(s.rows-1, i,' ');
	}

	attron(A_REVERSE);
	sprintf(msg, "Displaying: %s size: %dx%d", s.i_filepath, s.p_image.width, s.p_image.height);
	mvaddstr(0, 0, msg);
	sprintf(msg, "Terminal resolution: %dx%d | color: %s", s.cols, s.rows, s.color ? "ON " : "OFF");
	mvaddstr(s.rows-1, 0, msg);
	strcpy(msg, "PRESS q TO QUIT");
	mvaddstr(s.rows-1, s.cols-strlen(msg), msg);
	sprintf(msg, "scaling: %d%%" ,s.scaling);
	mvaddstr(s.rows-1, s.cols/2 - strlen(msg)/2, msg);
	attroff(A_REVERSE);
}

void draw_image(const screen s) {
	p_px p_pixel;

	for(int i=0; i < s.cols-1;i++) {
		for(int j=0; j < s.rows-1;j++) {
			if(i+s.init_col < s.sc_image.width && j+s.init_row < s.sc_image.height){
				p_pixel = s.sc_image.p_data[INDEX(i+s.init_col, j+s.init_row, s.sc_image.width)];
			} else {
				p_pixel = empty_px;
			}

			if(s.color)
				attron(COLOR_PAIR(p_pixel.color));
			mvaddch(j, i, p_pixel.chr | attrlist[p_pixel.attr]);
			if(s.color)
				attroff(COLOR_PAIR(p_pixel.color));
		}
	}
}

void apply_scaling(screen* s) {
	s->sc_image.width = SCALE(s->p_image.width,s->scaling);
	s->sc_image.height = SCALE(s->p_image.height,s->scaling);
	if(s->sc_image.p_data != NULL) {
		free(s->sc_image.p_data);
	}
	s->sc_image.p_data = malloc(sizeof(p_px)*s->sc_image.width*s->sc_image.height);
	if(s->sc_image.p_data == NULL){
		exit(1);
	}
	p_px pixel;
	for(int i=0; i< s->sc_image.width; i++) {
		for(int j=0; j<s->sc_image.height; j++) {
			pixel = s->p_image.p_data[INDEX(INV_SCALE(i,s->scaling),INV_SCALE(j,s->scaling),s->p_image.width)];
			s->sc_image.p_data[INDEX(i, j,s->sc_image.width)] = pixel;
		}
	}
}

void update_screen(screen* s, const int input) {
	getmaxyx(stdscr, s->rows, s->cols);

	switch (input) {
		case KEY_RIGHT:
			if((s->cols + s->init_col - 1 + NAV_SPEED) < s->p_image.width){
				s->init_col += NAV_SPEED;
			}
			break;
		case KEY_DOWN:
			if((s->rows + s->init_row - 1 + NAV_SPEED) < s->p_image.height){
				s->init_row += NAV_SPEED;
			}
			break;
		case KEY_LEFT:
			if(s->init_col - NAV_SPEED > 0){
				s->init_col -= NAV_SPEED;
			}
			break;
		case KEY_UP:
			if(s->init_row - NAV_SPEED > 0){
				s->init_row -= NAV_SPEED;
			}
			break;
		case 'c':
			s->color = !s->color;
			break;
		case 'z':
			if(s->scaling < 100){
				s->scaling++;
				apply_scaling(s);
				s->init_col = (s->sc_image.width + s->p_image.width/100)*s->init_col/s->sc_image.width;
				s->init_row = (s->sc_image.height + s->p_image.height/100)*s->init_row/s->sc_image.height;
			}
			break;
		case 'x':
			if(s->scaling > 1){
				s->scaling--;
				apply_scaling(s);
				s->init_col = (s->sc_image.width - s->p_image.width/100)*s->init_col/s->sc_image.width;
				s->init_row = (s->sc_image.height - s->p_image.height/100)*s->init_row/s->sc_image.height;
			}
			break;
		default:
			break;
	}
}

void init_screen(screen* s, const r_img img) {
	getmaxyx(stdscr, s->rows, s->cols);
	s->color = 0;
	s->init_col = 0;
	s->init_row = 0;
	s->i_filepath = img.filepath;
	s->scaling = s->rows*100/img.height;
	s->sc_image.p_data = NULL;
	s->p_image = process_r_image(img);
	apply_scaling(s);
}

void ncurses_setup() {
	initscr();
	keypad(stdscr, 1);
	clear();
	noecho();
	nodelay(stdscr,1);
	start_color();
	curs_set(0);

	for(int i=0; i<8;i++){
		init_pair(i, i, BLACK);
	}
}

void ncurses_close() {
	nocbreak();
	keypad(stdscr,0);
	echo();
	endwin();
}

void err_handler() {
	fprintf(stderr,"something went wrong xd");
	exit(1);
}

int main(int argc, char **argv) {
	char* filepath;
	if(argc < 2){
		err_handler();
	} else {
		filepath = argv[1];
	}

	r_img* img  = image_load(filepath);
	if (img == NULL) {
		err_handler();
	}

	ncurses_setup();

	screen s;
	s.sc_image.p_data = NULL;

	init_screen(&s, *img);
	image_free(img);
	uint32_t input = ERR;
	do {
		draw_image(s);
		draw_ui(s);
		update_screen(&s, input);
		refresh();
		usleep(1000000/FPS);
	} while((input = getch())!= QUIT_CHAR);

	ncurses_close();
	return 0;
}
