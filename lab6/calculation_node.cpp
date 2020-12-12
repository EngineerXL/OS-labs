#include <iostream>
#include <pthread.h>
#include <queue>
#include <tuple>
#include <unistd.h>

#include "search.hpp"
#include "zmq_std.hpp"

const std::string NODE_SENTINEL = "$";

long long node_id;
pthread_mutex_t mutex;
pthread_cond_t cond;
std::queue< std::pair<std::string, std::string> > calc_queue;

void* thread_func(void*) {
		while (1) {
			pthread_mutex_lock(&mutex);
			while (calc_queue.empty()) {
				pthread_cond_wait(&cond, &mutex);
			}
			std::pair<std::string, std::string> cur = calc_queue.front();
			calc_queue.pop();
			pthread_mutex_unlock(&mutex);
			if (cur.first == NODE_SENTINEL and cur.second == NODE_SENTINEL) {
				break;
			} else {
				std::vector<unsigned int> res = KMPStrong(cur.first, cur.second);
				std::cout << "OK: " << node_id << " : ";
				for (size_t i = 0; i < res.size() - 1; ++i) {
					std::cout << res[i] << ", ";
				}
				std::cout << res.back() << std::endl;
			}
		}
		return NULL;
	}

int main(int argc, char** argv) {
	int rc;
	assert(argc == 2);
	node_id = std::stoll(std::string(argv[1]));

	void* node_zmq_context = zmq_ctx_new();
	void* node_zmq_socket = zmq_socket(node_zmq_context, ZMQ_REP);
	rc = zmq_connect(node_zmq_socket, "tcp://localhost:4080");
	assert(rc == 0);
	
	void* node_text_zmq_context = zmq_ctx_new();
	void* node_text_zmq_socket = zmq_socket(node_text_zmq_context, ZMQ_SUB);
	rc = zmq_connect(node_text_zmq_socket, "tcp://localhost:1234");
	assert(rc == 0);
	rc = zmq_setsockopt(node_text_zmq_socket, ZMQ_SUBSCRIBE, NULL, 0);
	assert(rc == 0);

	std::cout << "OK: " << getpid() << std::endl;
	
	pthread_t calculation_thread;
	rc = pthread_mutex_init(&mutex, NULL);
	assert(rc == 0);
	rc = pthread_cond_init(&cond, NULL);
	assert(rc == 0);
	rc = pthread_create(&calculation_thread, NULL, thread_func, NULL);
	assert(rc == 0);

	bool awake = true;
	while (awake) {
		node_token_t token = zmq_std::recieve_msg<node_token_t>(node_zmq_socket);
		node_token_t* token_reply = new node_token_t;
		token_reply->action = token.action;
		token_reply->id = node_id;
		if (token.id == node_id) {
			if (token.action == destroy) {
				awake = false;
			} else if (token.action == calculate) {
				std::string pattern, text;
				bool flag_sentinel = true;
				while (1) {
					char c = zmq_std::recieve_msg<char>(node_text_zmq_socket);
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
				}
				pthread_mutex_lock(&mutex);
				if (calc_queue.empty()) {
					pthread_cond_signal(&cond);
				}
				calc_queue.push({pattern, text});
				pthread_mutex_unlock(&mutex);
			}
			token_reply->action = success;
		}
		zmq_std::send_msg(token_reply, node_zmq_socket);
	}
	rc = zmq_close(node_zmq_socket);
	assert(rc == 0);
	rc = zmq_close(node_text_zmq_socket);
	assert(rc == 0);

	pthread_mutex_lock(&mutex);
	if (calc_queue.empty()) {
		pthread_cond_signal(&cond);
	}
	calc_queue.push({NODE_SENTINEL, NODE_SENTINEL});
	pthread_mutex_unlock(&mutex);
	rc = pthread_join(calculation_thread, NULL);
	assert(rc == 0);

	rc = pthread_cond_destroy(&cond);
	assert(rc == 0);
	rc = pthread_mutex_destroy(&mutex);
	assert(rc == 0);

	rc = zmq_ctx_term(node_zmq_context);
	assert(rc == 0);
	rc = zmq_ctx_term(node_text_zmq_context);
	assert(rc == 0);
}