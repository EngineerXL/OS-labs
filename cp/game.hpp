#include <math.h>
#include <ncurses.h>
#include <random>
#include <stdexcept>
#include <time.h>
#include <unistd.h>

#include "zmq_std.hpp"

class game {
private:
	/* Determines bind or connect */
	bool mode;

	/*
	 * Playing field is rectangle with width FIELD_WIDTH
	 * and height FIELD_HEIGHT. Programm will adopt to
	 * user's temrinal LINES and COLS
	 */
	const unsigned int FIELD_FACTOR = 10;
	const unsigned int FIELD_HEIGHT = 9;
	const unsigned int FIELD_WIDTH = 16;

	const double PONG_SPEED = 4.0;
	const double BALL_SPEED = 1.5;
	const double FIELD_LEFT = 0;
	const double FIELD_UP = 0;
	const double FIELD_RIGHT = FIELD_LEFT + FIELD_WIDTH * FIELD_FACTOR;
	const double FIELD_DOWN = FIELD_UP + FIELD_HEIGHT * FIELD_FACTOR;
	const double PONG_SIZE = (FIELD_DOWN - FIELD_UP) / 5.0;

	const double SCALING_FACTOR_X = (LINES - 2) / (FIELD_DOWN - FIELD_UP);
	unsigned int transform_x(const double & cord) {
		return cord * SCALING_FACTOR_X + 1;
	}

	const double SCALING_FACTOR_Y = (COLS - 1) / (FIELD_RIGHT - FIELD_LEFT);
	unsigned int transform_y(const double & cord) {
		return cord * SCALING_FACTOR_Y;
	}

	struct game_info_t {
		double player1_pos = 0;
		double player2_pos = 0;
		double ball_pos_x = 0;
		double ball_pos_y = 0;
		double ball_vec_x = 0;
		double ball_vec_y = 0;
		unsigned long long score1 = 0;
		unsigned long long score2 = 0;
	};

	void gen_ball(game_info_t & info) {
		info.ball_pos_x = (FIELD_DOWN - FIELD_UP) / 2.0;
		info.ball_pos_y = (FIELD_RIGHT - FIELD_LEFT) / 2.0;
		double angle = (5236 + std::abs(rand()) % 5236) / 10000.0;
		info.ball_vec_x = BALL_SPEED * std::cos(angle);
		info.ball_vec_y = BALL_SPEED * std::sin(angle);
	}

	/* System call return code */
	int rc;

	WINDOW* window;
	WINDOW* player1;
	WINDOW* player2;

	void* context = NULL;
	void* socket = NULL;

public:
	game(const bool & input_mode, const std::string & IP) : mode(input_mode) {
		window = newwin(LINES, COLS, 0, 0);
		wborder(window, ' ', ' ', 0, 0, ACS_HLINE, ACS_HLINE, ACS_HLINE, ACS_HLINE);
		keypad(window, TRUE);
		nodelay(window, TRUE);
		wrefresh(window);

		context = zmq_ctx_new();
		if (context == NULL) {
			throw std::runtime_error("Error creating context!");
		}
		socket = zmq_socket(context, ZMQ_PAIR);
		if (socket == NULL) {
			throw std::runtime_error("Error creating socket!");
		}
		int linger_period = 1000;
		rc = zmq_setsockopt(socket, ZMQ_LINGER, &linger_period, sizeof(int));
		if (rc != 0) {
			throw std::runtime_error("Error setting linger!");	
		}
		if (mode) {
			rc = zmq_bind(socket, IP.c_str());
			if (rc != 0) {
				throw std::runtime_error("Cannot bind to " + IP);
			}
		} else {
			rc = zmq_connect(socket, IP.c_str());
			if (rc != 0) {
				throw std::runtime_error("Cannot connect to " + IP);
			}
		}
		std::srand(time(NULL));
	}

	void play() {
		game_info_t info1;
		game_info_t info2;
		if (mode) {
			gen_ball(info1);
		}
		info1.player1_pos = (FIELD_DOWN - FIELD_UP) / 2.0;
		info1.player2_pos = (FIELD_DOWN - FIELD_UP) / 2.0;

		int c;
		while (1) {
			usleep(30000);

			/* Game window and ball */
			wborder(window, ' ', ' ', 0, 0, ACS_HLINE, ACS_HLINE, ACS_HLINE, ACS_HLINE);
			mvwprintw(window, 0, COLS / 2 - 15, "> P1: %d <", info1.score1);
			mvwprintw(window, 0, COLS / 2 + 5, "> P2: %d <", info1.score2);
			mvwprintw(window, transform_x(info1.ball_pos_x), transform_y(info1.ball_pos_y), "O");
			wrefresh(window);

			/* Player 1 */
			player1 = newwin(transform_x(PONG_SIZE), 1, transform_x(info1.player1_pos), transform_y(FIELD_LEFT));
			wborder(player1, ' ', ACS_VLINE, ' ', ' ', ' ', ACS_URCORNER, ' ', ACS_LRCORNER);
			wrefresh(player1);

			/* Player 2 */
			player2 = newwin(transform_x(PONG_SIZE), 1, transform_x(info1.player2_pos), transform_y(FIELD_RIGHT) - 1);
			wborder(player2, ' ', ACS_VLINE, ' ', ' ', ' ', ACS_ULCORNER, ' ', ACS_LLCORNER);
			wrefresh(player2);

			c = wgetch(window);
			if (c == 10) {
				break;
			} else if (c == KEY_UP and info1.player1_pos >= FIELD_UP) {
				info1.player1_pos -= PONG_SPEED;
			} else if (c == KEY_DOWN and info1.player1_pos + PONG_SIZE <= FIELD_DOWN) {
				info1.player1_pos += PONG_SPEED;
			}

			if (mode) {
				if (info1.ball_pos_x + info1.ball_vec_x <= FIELD_UP) {
					info1.ball_vec_x = -info1.ball_vec_x;
				}

				if (info1.ball_pos_x + info1.ball_vec_x >= FIELD_DOWN) {
					info1.ball_vec_x = -info1.ball_vec_x;	
				}

				if (info1.ball_pos_y + info1.ball_vec_y <= FIELD_LEFT) {
					if (info1.player1_pos <= info1.ball_pos_x and info1.ball_pos_x <= info1.player1_pos + PONG_SIZE) {
						info1.ball_vec_y = -info1.ball_vec_y;
					} else {
						++info1.score2;
						gen_ball(info1);
					}
				}

				if (info1.ball_pos_y + info1.ball_vec_y >= FIELD_RIGHT) {
					if (info1.player2_pos <= info1.ball_pos_x and info1.ball_pos_x <= info1.player2_pos + PONG_SIZE) {
						info1.ball_vec_y = -info1.ball_vec_y;
					} else {
						++info1.score1;
						gen_ball(info1);
						info1.ball_vec_x = -info1.ball_vec_x;
						info1.ball_vec_y = -info1.ball_vec_y;
					}
				}

				info1.ball_pos_x += info1.ball_vec_x;
				info1.ball_pos_y += info1.ball_vec_y;
			}

			zmq_std::send_msg_dontwait(&info1, socket);

			/*
			 * Programm will recieve more messages to
			 * decrease ping emptying the queue
			 */
			for (int i = 0; i < 10; ++i) {
				if (zmq_std::recieve_msg_dontwait(info2, socket)) {
					info1.player2_pos = info2.player1_pos;
					if (!mode) {
						info1.score1 = info2.score2;
						info1.score2 = info2.score1;

						info1.ball_pos_x = info2.ball_pos_x;
						info1.ball_pos_y = FIELD_RIGHT - info2.ball_pos_y;

						info1.ball_vec_x = info2.ball_vec_x;
						info1.ball_vec_y = -info2.ball_vec_y;
					}
				}
			}
			delwin(player1);
			delwin(player2);
			wclear(window);
		}
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