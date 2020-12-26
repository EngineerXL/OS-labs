#include <cstdio>
#include <ncurses.h>
#include <string>
#include <vector>

#include "game.hpp"
#include "zmq_std.hpp"

const char* LOG_NAME = "log.txt";
void* context = NULL;
void* socket = NULL;
FILE* log_file = NULL;

// void game_connect(const std::string & IP) {
// 	int rc;
// 	context = zmq_ctx_new();
// 	assert(context != NULL);
// 	socket = zmq_socket(context, ZMQ_PAIR);
// 	assert(socket != NULL);
// 	rc = zmq_connect(socket, IP.c_str());
// 	if (rc != 0) {
// 		fprintf(log_file, "Error connecting to %s\n", IP.c_str());
// 		return;
// 	}

// 	/* Recieve message */
// 	int res;
// 	zmq_std::recieve_msg(res, socket);

// 	FILE* log_file = fopen(LOG_NAME, "w");
// 	fprintf(log_file, "Got it %d\n", res);
// 	fclose(log_file);

// 	rc = zmq_close(socket);
// 	assert(rc == 0);
// 	rc = zmq_ctx_term(context);
// 	assert(rc == 0);
// }

// void game_bind(const std::string & IP) {
// 	int rc;
// 	void* context = NULL;
// 	context = zmq_ctx_new();
// 	assert(context != NULL);
// 	void* socket = NULL;
// 	socket = zmq_socket(context, ZMQ_PAIR);
// 	assert(socket != NULL);
// 	rc = zmq_bind(socket, IP.c_str());
// 	if (rc != 0) {
// 		fprintf(log_file, "Error binding to %s\n", IP.c_str());
// 		return;
// 	}

// 	/* Send message */
// 	int* res = new int(123456789);
// 	zmq_std::send_msg(res, socket);

// 	rc = zmq_close(socket);
// 	assert(rc == 0);
// 	rc = zmq_ctx_term(context);
// 	assert(rc == 0);
// }

int main() {
	log_file = fopen(LOG_NAME, "w");

	initscr();
	raw();
	refresh();
	int start_x = 0, start_y = 0, end_x = LINES, end_y = COLS;
	int center_x = (end_x - start_x) / 2, center_y = (end_y - start_y) / 2;
	WINDOW* menu = newwin(end_x - start_x, end_y - start_y, start_x, start_y);
	box(menu, 0, 0);
	wrefresh(menu);
	std::vector< std::string > menu_items = {
		"Create game",
		"Connect to existing game",
		"Exit"
	};
	const size_t new_game_ind = 0, connect_ind = 1, exit_ind = 2;
	(void)connect_ind;

	noecho();
	keypad(menu, TRUE);
	curs_set(0);


	int c;
	size_t menu_item_ind = 0;
	for (size_t i = 0; i < menu_items.size(); ++i) {
		if (i == menu_item_ind) {
			wattron(menu, A_STANDOUT);
		} else {
			wattroff(menu, A_STANDOUT);
		}
		mvwprintw(menu, center_x + i * 2, center_y, menu_items[i].c_str());
	}
	wrefresh(menu);

	fprintf(log_file, "Start log...\n");

	while(1) {
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
				WINDOW* port_menu = newwin(10, 25, center_x, center_y);
				box(port_menu, 0, 0);
				wrefresh(port_menu);
				std::string port;
				bool ok = true;
				echo();
				while (ok) {
					c = wgetch(menu);
					if (c == 27) {
						ok = false;
					} else if (c == 10) {
						break;
					} else {
						port = port + (char)c;
					}
				}
				noecho();
				delwin(port_menu);
				if (ok) {
					fprintf(log_file, "Creating game on port %s\n", port.c_str());
					game new_game(1, std::string("tcp://*:") + port);
					// std::thread game_thread(game_bind, std::string("tcp://*:") + port);
					new_game.play();
					// game_thread.join();
				}
			} else if (menu_item_ind == connect_ind) {
				/*
				 * Connecting to existing game
				 */
				WINDOW* ip_menu = newwin(10, 25, center_x, center_y);
				box(ip_menu, 0, 0);
				wrefresh(ip_menu);
				std::string ip;
				bool ok = true;
				echo();
				while (ok) {
					c = wgetch(menu);
					if (c == 27) {
						ok = false;
					} else if (c == 10) {
						break;
					} else {
						ip = ip + (char)c;
					}
				}
				noecho();
				delwin(ip_menu);
				if (ok) {
					fprintf(log_file, "Connecting to %s\n", ip.c_str());
					game new_game(false, std::string("tcp://") + ip);
					// fprintf(log_file, "%s\n", (std::string("tcp://") + ip).c_str());
					// std::thread game_thread(game_connect, std::string("tcp://") + ip);
					new_game.play();
					// game_thread.join();
				}
			}
		}
		fprintf(log_file, "Going to redraw\n");
		wclear(menu);
		wrefresh(menu);
		box(menu, 0, 0);
		for (size_t i = 0; i < menu_items.size(); ++i) {
			if (i == menu_item_ind) {
				wattron(menu, A_STANDOUT);
			} else {
				wattroff(menu, A_STANDOUT);
			}
			mvwprintw(menu, center_x + i * 2, center_y, menu_items[i].c_str());
		}
		wattroff(menu, A_STANDOUT);
		refresh();
	}
	delwin(menu);
	endwin();
	fclose(log_file);
}