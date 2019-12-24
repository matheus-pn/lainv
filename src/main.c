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

void err_handle() {
	fprintf(stderr,"something went wrong xd");
	exit(1);
}

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} rgb_data;


char* filename;

int main(int argc, char **argv) {
	if(argc < 2){
		err_handle();
	}else {
		filename = argv[1];
	}

	int size_x, size_y, n;
	rgb_data* image_buffer = (rgb_data*)stbi_load(filename, &size_x, &size_y, &n, 0);
	if (image_buffer == NULL) {
		err_handle();
	}

	ncurses_setup();
	int rows, cols, input = ERR;
	int start_x = 0, start_y = 0;
	char msg[1000];
	rgb_data pixel;
	do {
		refresh();
		getmaxyx(stdscr,rows,cols);

		for(int i = 0; i < cols-1; i++){
			for(int j = 0; j < rows-1; j++){
				pixel = image_buffer[(j+start_y)*size_x+(i+start_x)];
				int sum = pixel.red + pixel.green + pixel.blue;
				char chr;
				ulong attr;
				if(sum > 377){
					chr = 'g';
					attr = A_BOLD;
				} else {
					chr = '.';
					attr = A_DIM;
				}
				mvaddch(j,i, chr | attr);
			}
		}
		attron(A_REVERSE);
		sprintf(msg,"Displaying: %s size: %dx%d",filename,size_x,size_y);
		mvaddstr(0,0,msg);
		sprintf(msg,"Terminal resolution: %dx%d",cols,rows);
		mvaddstr(rows-1, 0, msg);
		strcpy(msg,"PRESS q TO QUIT");
		mvaddstr(rows-1, cols-strlen(msg), msg);
		sprintf(msg,"x: %d/%d | y: %d/%d",cols+start_x-1,size_x,rows+start_y-1,size_y);
		mvaddstr(rows-1,cols/2 - strlen(msg)/2, msg);
		attroff(A_REVERSE);

		switch (input) {
			case KEY_RIGHT:
				if((cols + start_x - 1 + NAV_SPEED) < size_x){
					start_x += NAV_SPEED;
				}
				break;
			case KEY_DOWN:
				if((rows + start_y - 1 + NAV_SPEED) < size_y){
					start_y += NAV_SPEED;
				}
				break;
			case KEY_LEFT:
				if(start_x - NAV_SPEED > 0){
					start_x -= NAV_SPEED;
				}
				break;
			case KEY_UP:
				if(start_y - NAV_SPEED > 0){
					start_y -= NAV_SPEED;
				}
				break;
			default:
				break;
		}
	} while((input = getch())!='q');

	ncurses_close();
	return 0;
}
