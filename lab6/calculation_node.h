#ifndef CALCULATION_NODE
#define CALCULATION_NODE

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <zmq.h>
#include <pthread.h>
#include <queue>
#include <tuple>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>
#include <string.h>
#include <errno.h>

enum actions_t {
	idle = 0,
	destroy = 1,
	ping = 2,
	calculate = 3
};

struct node_token_t {
	actions_t action;
	long long id;
};

pthread_mutex_t mutex;
pthread_cond_t cond;
std::queue< std::pair<std::string, std::string> > calc_queue;

class calculation_node_t {
private:
	long long node_id;
	void* node_zmq_context;
	void* node_zmq_socket;
	pthread_t calculation_thread;
	void* node_text_zmq_context;
	void* node_text_zmq_socket;

	// static pthread_mutex_t mutex;
	// static pthread_cond_t cond;
	// static std::queue< std::pair<std::string, std::string> > calc_queue;

	/* System call return code */
	int rc;

	node_token_t recieve_msg() {
		zmq_msg_t message;
		zmq_msg_init(&message);
		rc = zmq_msg_recv(&message, node_zmq_socket, 0);
		assert(rc == sizeof(node_token_t));
		node_token_t token = *(node_token_t*)zmq_msg_data(&message);
		rc = zmq_msg_close(&message);
		assert(rc == 0);
		return token;
	}

	void send_msg(node_token_t* token) {
		zmq_msg_t message;
		zmq_msg_init(&message);
		rc = zmq_msg_init_size(&message, sizeof(node_token_t));
		assert(rc == 0);
		rc = zmq_msg_init_data(&message, token, sizeof(node_token_t), NULL, NULL);
		assert(rc == 0);
		rc = zmq_msg_send(&message, node_zmq_socket, 0);
		assert(rc == sizeof(node_token_t));
		rc = zmq_msg_close(&message);
		assert(rc == 0);
	}

	struct thread_token_t {
		std::queue< std::pair<std::string, std::string> > token_queue;
		pthread_mutex_t token_mutex;
		pthread_cond_t token_cond;
	};

	static void* thread_func(void*) {
		// thread_token_t* token = (thread_token_t*)args;
		// std::queue< std::pair<std::string, std::string> > queue = token->token_queue;
		// pthread_mutex_t mutex = token->token_mutex;
		// pthread_cond_t cond = token->token_cond;
		// std::cout << "----- > THREAD 1" << std::endl;
		while (1) {
			pthread_mutex_lock(&mutex);
			while (calc_queue.empty()) {
				std::cout << "----- > THREAD WAIT" << std::endl;
				pthread_cond_wait(&cond, &mutex);
			}
			std::pair<std::string, std::string> cur = calc_queue.front();
			calc_queue.pop();
			pthread_mutex_unlock(&mutex);
			std::cout << "----- > THREAD GOT " << cur.first << " - " << cur.second << std::endl;
			if (cur.first == "y" and cur.second == "y") {
				break;
			} else {
				std::cout << "Thread will search " << cur.first << " in " << cur.second << std::endl;
			}
		}
		std::cout << "----- > THREAD 2" << std::endl;
		return NULL;
	}

public:
	explicit calculation_node_t(const long long & id) : node_id(id) {
		// static pthread_mutex_t mutex;
		// static pthread_cond_t cond;
		// static std::queue< std::pair<std::string, std::string> > calc_queue;

		std::cout << "Calling constructor [" << node_id << "]" << std::endl;
		node_zmq_context = zmq_ctx_new();
		assert(node_zmq_context != NULL);
		node_zmq_socket = zmq_socket(node_zmq_context, ZMQ_REP);
		assert(node_zmq_socket != NULL);
		// pthread_mutexattr_t mutex_attr;
		// rc = pthread_mutexattr_init(&mutex_attr);
		node_text_zmq_context = zmq_ctx_new();
		assert(node_zmq_context != NULL);
		node_text_zmq_socket = zmq_socket(node_text_zmq_context, ZMQ_SUB);
		assert(node_zmq_socket != NULL);
		// assert(rc == 0);
		// rc = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_PRIVATE);
		// assert(rc == 0);
		rc = pthread_mutex_init(&mutex, NULL);
		assert(rc == 0);

		// rc = pthread_mutexattr_destroy(&mutex_attr);
		// assert(rc == 0);

		rc = pthread_cond_init(&cond, NULL);
		assert(rc == 0);

		// std::cout << "---> I'm here!!!" << std::endl;
		thread_token_t* token = new thread_token_t;
		token->token_queue = calc_queue;
		token->token_mutex = mutex;
		rc = pthread_create(&calculation_thread, NULL, calculation_node_t::thread_func, token);
		// std::cout << "Error: " << rc << std::endl;
		// std::cout << strerror(errno) << std::endl;
		assert(rc == 0);
		// std::cout << "Calling constructor [" << node_id << "]" << std::endl;
		rc = zmq_connect(node_zmq_socket, "tcp://localhost:4080");
		assert(rc == 0);
		rc = zmq_connect(node_text_zmq_socket, "tcp://localhost:1234");
		assert(rc == 0);
		rc = zmq_setsockopt(node_text_zmq_socket, ZMQ_SUBSCRIBE, NULL, 0);
		assert(rc == 0);
	}
	
	int execute() {
		// std::cout << "OK: " << getpid() << std::endl;

		std::cout << "Created! MyID = " << node_id << std::endl;
		bool awake = true;
		while (awake) {
			std::cout << "Alive! " << getpid() << std::endl;
			node_token_t token = recieve_msg();
			{
				std::cout << "Got message! " << getpid() << std::endl;
				std::cout << "My ID = " << node_id << "; Msg ID = " << token.id << std::endl;
			}
			node_token_t* token_reply = new node_token_t;
			token_reply->action = token.action;
			token_reply->id = node_id;
			if (token.action == destroy) {
				// std::cout << "I'm here1" << std::endl;
				if (token.id == node_id) {
					token_reply->action = idle;
					// std::cout << "I'm here2" << std::endl;
					// zmq_msg_init_data(&message, &token, sizeof(node_token_t), NULL, NULL);
					awake = false;
				}
			} else if (token.action == ping) {
				if (token.id == node_id) {
					token_reply->action = idle;
				}
			} else if (token.action == calculate and token.id == node_id) {
				// int fd = token.fd;
				std::string pattern, text;
				bool flag_sentinel = true;
				while (1) {
					zmq_msg_t message;
					zmq_msg_init(&message);
					// std::cout << "---> I block here" << std::endl;
					rc = zmq_msg_recv(&message, node_text_zmq_socket, 0);
					assert(rc == sizeof(char));
					char c = *(char*)zmq_msg_data(&message);
					rc = zmq_msg_close(&message);
					assert(rc == 0);
					if (c == '$') {
						if (flag_sentinel) {
							flag_sentinel = 0;
							std::swap(text, pattern);
						} else {
							break;
						}
					} else {
						text += c;
					}
					// return token;
				}
				// assert(rc == 0);
				// std::cin >> pattern >> text;
				pthread_mutex_lock(&mutex);
				if (calc_queue.empty()) {
					pthread_cond_signal(&cond);
				}
				calc_queue.push({pattern, text});
				pthread_mutex_unlock(&mutex);
				token_reply->action = idle;
			}
			while (1) {
				;
			}
			send_msg(token_reply);
		}
		rc = zmq_close(node_zmq_socket);
		assert(rc == 0);
		rc = zmq_close(node_text_zmq_socket);
		assert(rc == 0);
		// std::cout << getpid() << " [" << node_id << "] exited normally 1" << std::endl;
		return 0;
	}

	~calculation_node_t() {
		std::cout << "Calling destructor [" << node_id << "]" << std::endl;
		pthread_mutex_lock(&mutex);
		if (calc_queue.empty()) {
			pthread_cond_signal(&cond);
		}
		// std::cout << "Calling destructor [" << node_id << "]" << std::endl;
		calc_queue.push({"y", "y"});
		// std::cout << "Calling destructor [" << node_id << "]" << std::endl;
		rc = pthread_cond_signal(&cond);
		// std::cout << "Calling destructor [" << node_id << "]" << std::endl;
		assert(rc == 0);
		pthread_mutex_unlock(&mutex);
		// std::cout << "Calling destructor [" << node_id << "]" << std::endl;

		rc = pthread_join(calculation_thread, NULL);
		// std::cout << "Calling destructor [" << node_id << "]" << std::endl;
		assert(rc == 0);
		
		rc = pthread_cond_destroy(&cond);
		assert(rc == 0);

		rc = pthread_mutex_destroy(&mutex);
		assert(rc == 0);

		rc = zmq_ctx_term(node_zmq_context);
		assert(rc == 0);
		rc = zmq_ctx_term(node_text_zmq_context);
		assert(rc == 0);
		std::cout << "---------- ---------- ---------- ---------- > Destroyed [" << node_id << "]" << std::endl;
	}
};

#endif /* CALCULATION_NODE */