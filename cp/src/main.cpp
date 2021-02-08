#include <iostream>
#include <ncurses.h>
#include <stdio.h>
#include <vector>

#include "game.hpp"
#include "zmq_std.hpp"

std::string read_cords(unsigned int x, unsigned int y) {
	std::string res;
	bool ok = true;
	int c;
	echo();
	WINDOW* window = newwin(4, 10, x, y);
	while (ok) {
		c = wgetch(window);
		if (c == 27) {
			ok = false;
		} else if (c == 10) {
			break;
		} else {
			res = res + (char)c;
		}
	}
	noecho();
	delwin(window);
	if (ok) {
		return res;
	} else {
		return std::string();
	}
}

int main() {
	const char* LOG_NAME = "log.txt";
	FILE* log_file = NULL;
	log_file = fopen(LOG_NAME, "w");
	if (log_file == NULL) {
		std::cout << "Error creating log file!" << std::endl;
		return -1;
	}
	fprintf(log_file, "Starting log...\n");
	initscr();
	if (LINES < 20 or COLS < 80) {
		std::cout << "Invalid terminal size!" << std::endl;
		endwin();
		fclose(log_file);
		return -1;
	}
	/* Disable line buffering */
	raw();
	const unsigned int start_x = 0;
	const unsigned int start_y = 0;
	const unsigned int end_x = LINES;
	const unsigned int end_y = COLS;
	const unsigned int center_x = (end_x - start_x) / 2;
	const unsigned int center_y = (end_y - start_y) / 2;
	std::vector< std::string > menu_items = {
		"Create game",
		"Connect to existing game",
		"Exit"
	};
	const size_t new_game_ind = 0;
	const size_t connect_ind = 1;
	const size_t exit_ind = 2;

	WINDOW* menu = newwin(end_x - start_x, end_y - start_y, start_x, start_y);
	/* Do not print wgetch() */
	noecho();
	/* Enables window input */
	keypad(menu, TRUE);
	/* Hide cursor */
	curs_set(0);

	size_t menu_item_ind = 0;
	int c;
	while(1) {
		box(menu, 0, 0);
		for (size_t i = 0; i < menu_items.size(); ++i) {
			if (i == menu_item_ind) {
				/* Highlight current */
				wattron(menu, A_STANDOUT);
			}
			mvwprintw(menu, center_x + i * 2, center_y - 12, menu_items[i].c_str());
			wattroff(menu, A_STANDOUT);
		}
		wrefresh(menu);

		c = wgetch(menu);
		fprintf(log_file, "Getting key... %d\n", c);
		if (c == KEY_DOWN) {
			if (menu_item_ind < menu_items.size() - 1) {
				++menu_item_ind;
			}
		} else if (c == KEY_UP) {
			if (menu_item_ind > 0) {
				--menu_item_ind;
			}
		} else if (c == 10) {
			if (menu_item_ind == exit_ind) {
				break;
			} else if (menu_item_ind == new_game_ind) {
				/*
				 * Creating new game
				 */
				WINDOW* port_menu = newwin(10, 30, center_x - 1, center_y - 15);
				box(port_menu, 0, 0);
				wrefresh(port_menu);
				std::string port = read_cords(center_x + 1, center_y - 5);
				delwin(port_menu);
				if (!port.empty()) {
					fprintf(log_file, "Creating game on port %s\n", port.c_str());
					try {
						game new_game(true, std::string("tcp://*:") + port);
						new_game.play();
					} catch (std::exception & ex) {
						fprintf(log_file, "%s\n", ex.what());
					}
				}
			} else if (menu_item_ind == connect_ind) {
				/*
				 * Connecting to existing game
				 */
				WINDOW* ip_menu = newwin(10, 30, center_x - 1, center_y - 15);
				box(ip_menu, 0, 0);
				wrefresh(ip_menu);
				std::string ip = read_cords(center_x + 1, center_y - 5);
				delwin(ip_menu);
				if (!ip.empty()) {
					fprintf(log_file, "Connecting to %s\n", ip.c_str());
					try {
						game new_game(false, std::string("tcp://") + ip);
						new_game.play();
					} catch (std::exception & ex) {
						fprintf(log_file, "%s\n", ex.what());
					}
				}
			}
		}
		wclear(menu);
		wrefresh(menu);
		refresh();
	}
	delwin(menu);
	endwin();
	fprintf(log_file, "End of log...\n");
	fclose(log_file);
}