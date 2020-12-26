#include <assert.h>
#include <mutex>
#include <ncurses.h>
#include <thread>
#include <zmq.h>

#include <unistd.h>

#include <functional>

#include "zmq_std.hpp"

class game {
private:
	std::mutex mutex;
	bool mode;

	bool awake;

	size_t start_cord = (window_start_x + window_height) / 2;
	size_t start_cord2 = (window_start_x + window_height) / 2;

	// struct game_info_t {
	// 	size_t player1_pos;
	// 	size_t player2_pos;
	// 	long double ball_pos_x;
	// 	long double ball_pos_y;
	// 	long double ball_vec_x;
	// 	long double ball_vec_y;
	// };

	WINDOW* window;
	size_t window_start_x = 0, window_start_y = 0;
	size_t window_height = LINES, window_width = COLS;

	int rc;
	void* context = NULL;
	void* socket = NULL;

public:
	game(const bool & input_mode, const std::string & IP) : mode(input_mode) {
		window = newwin(window_height, window_width, window_start_x, window_start_y);
		wborder(window, ' ', ' ', 0, 0, ACS_HLINE, ACS_HLINE, ACS_HLINE, ACS_HLINE);
		keypad(window, TRUE);
		mvwprintw(window, 2, 2, "LOL");
		wrefresh(window);

		nodelay(window, TRUE);

		context = zmq_ctx_new();
		assert(context != NULL);
		socket = zmq_socket(context, ZMQ_PAIR);
		assert(socket != NULL);
		if (mode) {
			rc = zmq_bind(socket, IP.c_str());
		} else {
			rc = zmq_connect(socket, IP.c_str());
		}
		// assert(rc == 0);
	}

	void play() {
		size_t pong_width = 10;
		int c;
		// bool awake;

		// std::function<void()> thread_send = [&]() {
		// 	while (awake) {
		// 		mutex.lock();
				
		// 		mutex.unlock();
		// 	}
		// };

		// std::function<void()> thread_recieve = [&]() {
		// 	while (awake) {
		// 		mutex.lock();
				
		// 		mutex.unlock();
		// 	}
		// };

		// std::thread send(thread_send);
		// std::thread recieve(thread_recieve);

		WINDOW* player = newwin(pong_width, 1, start_cord, window_start_y);
		WINDOW* player2 = newwin(pong_width, 1, start_cord2, window_start_y + window_height - 1);

		while (1) {
			c = wgetch(window);
			usleep(25000);

			wclear(window);
			wborder(window, ' ', ' ', 0, 0, ACS_HLINE, ACS_HLINE, ACS_HLINE, ACS_HLINE);
			wrefresh(window);
			if (c == 10) {
				break;
			} else if (c == KEY_UP and start_cord > window_start_x + 1) {
				// mutex.lock();
				--start_cord;
				// mutex.unlock();
			} else if (c == KEY_DOWN and start_cord + pong_width < window_start_x + window_height - 1) {
				// mutex.lock();
				++start_cord;
				// mutex.unlock();
			}
			zmq_std::send_msg_dontwait(&start_cord, socket);

			zmq_std::recieve_msg_dontwait(start_cord2, socket);

			// if (mode) {
			// 	ball_pos_x += ball_vec_x;
			// 	ball_pos_y += ball_pos_y;
			// }

			box(player, 0, 0);
			wrefresh(player);

			box(player2, 0, 0);
			wrefresh(player2);

			delwin(player);
			mutex.lock();
			player = newwin(pong_width, 1, start_cord, window_start_y);
			mutex.unlock();
			box(player, 0, 0);
			wrefresh(player);

			delwin(player2);
			mutex.lock();
			player2 = newwin(pong_width, 1, start_cord2, window_start_y + window_width - 1);
			mutex.unlock();
			box(player2, 0, 0);
			wrefresh(player2);
		}

		delwin(player);
		delwin(player2);
	}

	~game() {
		rc = zmq_close(socket);
		assert(rc == 0);
		rc = zmq_ctx_term(context);
		assert(rc == 0);

		wrefresh(window);
		delwin(window);
	}
};