/*
 * main.c
 *
 *  Created on: Dec 23, 2019
 *      Author: Matheus Pereira Nogueira
 */


#include <ncurses.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



#define NAV_SPEED 4
#define MAX_SCREEN 1000

typedef struct rgb_data {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb_data;

typedef struct image {
	rgb_data* image_data;
	const char* filepath;
	int width;
	int heigth;
	int channels;
} image;

typedef struct screen {
	int cols;
	int rows;
	int start_row;
	int start_col;
	const char* curr_filepath;
	int curr_image_width;
	int curr_image_height;
	chtype* processed_image;
} screen;

const image* image_open(const char* filepath){
	image* img = malloc(sizeof(image));
	img->filepath = filepath;
	img->image_data = (rgb_data*)stbi_load(
			img->filepath,
			&(img->width),
			&(img->heigth),
			&(img->channels),
			0
	);
	return img;
}

chtype pixel_char(rgb_data pixel) {
	int sum = pixel.red + pixel.green + pixel.blue;
	chtype chr;
	if(sum > 510) {
		chr = 'g' | A_BOLD;
	} else if (sum > 255 && sum <= 510){
		chr = '/' | A_NORMAL;
	} else{
		chr = '.' | A_DIM;
	}
	return chr;
}

chtype* process_image(const image* image) {
	chtype* processed_image_buf = malloc(sizeof(chtype)*image->width*image->heigth);
	rgb_data raw_pixel;
	for(int i=0; i < image->width; i++){
		for(int j=0; j < image->heigth; j++){
			raw_pixel = image->image_data[j*image->width + i];
			processed_image_buf[j*image->width + i] = pixel_char(raw_pixel);
		}
	}
	return processed_image_buf;
}

void draw_ui(screen screen) {
	char msg[MAX_SCREEN];
	int size_x = screen.curr_image_width;
	int size_y = screen.curr_image_height;
	int cols = screen.cols;
	int rows = screen.rows;
	int start_x = screen.start_col;
	int start_y = screen.start_row;

	attron(A_REVERSE);
	sprintf(msg, "Displaying: %s size: %dx%d", screen.curr_filepath, size_x, size_y);
	mvaddstr(0, 0, msg);
	sprintf(msg, "Terminal resolution: %dx%d", cols, rows);
	mvaddstr(rows-1, 0, msg);
	strcpy(msg, "PRESS q TO QUIT");
	mvaddstr(rows-1, cols-strlen(msg), msg);
	sprintf(msg, "x: %d/%d | y: %d/%d", cols+start_x-1, size_x, rows+start_y-1, size_y);
	mvaddstr(rows-1, cols/2 - strlen(msg)/2, msg);
	attroff(A_REVERSE);
}

void draw_image(screen screen) {
	chtype px_char;
	int start_x = screen.start_col;
	int start_y = screen.start_row;
	int size_x = screen.curr_image_width;
	for(int i=0; i < screen.cols-1;i++){
		for(int j=0; j < screen.rows-1;j++){
			px_char = screen.processed_image[(j+start_y)*size_x+(i+start_x)];
			mvaddch(j, i, px_char);
		}
	}
}
void update_navigation(int input, screen* screen){
	int size_x = screen->curr_image_width;
	int size_y = screen->curr_image_height;
	int start_x = screen->start_col;
	int start_y = screen->start_row;


	switch (input) {
		case KEY_RIGHT:
			if((screen->cols + start_x - 1 + NAV_SPEED) < size_x){
				screen->start_col += NAV_SPEED;
			}
			break;
		case KEY_DOWN:
			if((screen->rows + start_y - 1 + NAV_SPEED) < size_y){
				screen->start_row += NAV_SPEED;
			}
			break;
		case KEY_LEFT:
			if(start_x - NAV_SPEED > 0){
				screen->start_col -= NAV_SPEED;
			}
			break;
		case KEY_UP:
			if(start_y - NAV_SPEED > 0){
				screen->start_row -= NAV_SPEED;
			}
			break;
		default:
			break;
	}
}

void ncurses_setup() {
	initscr();
	keypad(stdscr, 1);
	clear();
	noecho();
	nodelay(stdscr,1);
	start_color();
	curs_set(0);
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

	const image* image  = image_open(filepath);
	if (image == NULL) {
		err_handler();
	}

	ncurses_setup();

	screen screen;
	screen.start_col = 0;
	screen.start_row = 0;
	screen.curr_filepath = filepath;
	screen.curr_image_width = image->width;
	screen.curr_image_height = image->heigth;
	screen.processed_image = process_image(image);

	int input = ERR;
	do {
		refresh();
		getmaxyx(stdscr,screen.rows, screen.cols);
		draw_image(screen);
		draw_ui(screen);
		update_navigation(input, &screen);
	} while((input = getch())!='q');

	ncurses_close();
	return 0;
}
