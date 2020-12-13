#include <iostream>
#include <pthread.h>
#include <queue>
#include <tuple>
#include <unistd.h>

#include "search.hpp"
#include "zmq_std.hpp"

const std::string SENTINEL_STR = "$";

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
			if (cur.first == SENTINEL_STR and cur.second == SENTINEL_STR) {
				break;
			} else {
				std::vector<unsigned int> res = KMPStrong(cur.first, cur.second);
				std::cout << "OK: " << node_id << " : ";
				if (res.empty()) {
                    std::cout << "No matches" << std::endl;
                } else {
                    for (size_t i = 0; i < res.size() - 1; ++i) {
                        std::cout << res[i] << ", ";
                    }
                    std::cout << res.back() << std::endl;
                }
			}
		}
		return NULL;
	}

int main(int argc, char** argv) {
	int rc;
	assert(argc == 2);
	node_id = std::stoll(std::string(argv[1]));

	void* node_parent_context = zmq_ctx_new();
	void* node_parent_socket = zmq_socket(node_parent_context, ZMQ_PAIR);
	rc = zmq_connect(node_parent_socket, ("tcp://localhost:" + std::to_string(PORT_BASE + node_id)).c_str());
	assert(rc == 0);

	long long child_id = -1;
	void* node_context = NULL;
	void* node_socket = NULL;
	
	pthread_t calculation_thread;
	rc = pthread_mutex_init(&mutex, NULL);
	assert(rc == 0);
	rc = pthread_cond_init(&cond, NULL);
	assert(rc == 0);
	rc = pthread_create(&calculation_thread, NULL, thread_func, NULL);
	assert(rc == 0);

	std::string pattern, text;
	bool flag_sentinel = true;

	std::cout << "OK: " << getpid() << std::endl;

	bool has_child = false;
	bool awake = true;
	while (awake) {
		node_token_t token;
		zmq_std::recieve_msg(token, node_parent_socket);
		node_token_t* reply = new node_token_t({fail, node_id, node_id});

		if (token.action == bind and token.parent_id == node_id) {
			/*
			 * Bind could be recieved when parent created node
			 * and this node should bind to parent's child
			 */
			zmq_std::init_pair_socket(node_context, node_socket);
			rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str());
			assert(rc == 0);
			has_child = true;
			child_id = token.id;
			node_token_t* token_ping = new node_token_t({ping, child_id, child_id});
			node_token_t reply_ping({fail, child_id, child_id});
			if (zmq_std::send_recieve_wait(token_ping, reply_ping, node_socket) and reply_ping.action == success) {
				reply->action = success;
			}
		} else if (token.action == create) {
			if (token.parent_id == node_id) {
				if (has_child) {
					rc = zmq_close(node_socket);
					assert(rc == 0);
					rc = zmq_ctx_term(node_context);
					assert(rc == 0);
				}
				zmq_std::init_pair_socket(node_context, node_socket);
				rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str());
				assert(rc == 0);

				int fork_id = fork();
				if (fork_id == 0) {
					rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), NULL);
					assert(rc != -1);
					return 0;
				} else {
					bool ok = true;
					if (has_child) {
						node_token_t* token_bind = new node_token_t({bind, token.id, child_id});
						node_token_t reply_bind({fail, token.id, token.id});
						ok = zmq_std::send_recieve_wait(token_bind, reply_bind, node_socket);
						ok = ok and (reply_bind.action == success);
					}
					if (ok) {
						/* We should check if child has connected to this node */
						node_token_t* token_ping = new node_token_t({ping, token.id, token.id});
						node_token_t reply_ping({fail, token.id, token.id});
						ok = zmq_std::send_recieve_wait(token_ping, reply_ping, node_socket);
						ok = ok and (reply_ping.action == success);
						if (ok) {
							reply->action = success;
							child_id = token.id;
							has_child = true;
						} else {
							rc = zmq_close(node_socket);
							assert(rc == 0);
							rc = zmq_ctx_term(node_context);
							assert(rc == 0);
						}
					}
				}
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		} else if (token.action == ping) {
			if (token.id == node_id) {
				reply->action = success;
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		} else if (token.action == destroy) {
			if (has_child) {
				if (token.id == child_id) {
					bool ok = true;
					node_token_t* token_down = new node_token_t({destroy, node_id, child_id});
					node_token_t reply_down({fail, child_id, child_id});
					ok = zmq_std::send_recieve_wait(token_down, reply_down, node_socket);
					/* We should get special reply from child */
					if (reply_down.action == destroy and reply_down.parent_id == child_id) {
						rc = zmq_close(node_socket);
						assert(rc == 0);
						rc = zmq_ctx_term(node_context);
						assert(rc == 0);
						has_child = false;
						child_id = -1;
					} else if (reply_down.action == bind and reply_down.parent_id == node_id) {
						rc = zmq_close(node_socket);
						assert(rc == 0);
						rc = zmq_ctx_term(node_context);
						assert(rc == 0);
						zmq_std::init_pair_socket(node_context, node_socket);
						rc = zmq_bind(node_socket, ("tcp://*:" + std::to_string(PORT_BASE + reply_down.id)).c_str());
						assert(rc == 0);
						child_id = reply_down.id;
						node_token_t* token_ping = new node_token_t({ping, child_id, child_id});
						node_token_t reply_ping({fail, child_id, child_id});
						if (zmq_std::send_recieve_wait(token_ping, reply_ping, node_socket) and reply_ping.action == success) {
							ok = true;
						}
					}
					if (ok) {
						reply->action = success;
					}
				} else if (token.id == node_id) {
					rc = zmq_close(node_socket);
					assert(rc == 0);
					rc = zmq_ctx_term(node_context);
					assert(rc == 0);
					has_child = false;
					reply->action = bind;
					reply->id = child_id;
					reply->parent_id = token.parent_id;
					awake = false;
				} else {
					node_token_t* token_down = new node_token_t(token);
					node_token_t reply_down(token);
					reply_down.action = fail;
					if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
						*reply = reply_down;
					}
				}
			} else if (token.id == node_id) {
				/* Special message to parent */
				reply->action = destroy;
				reply->parent_id = node_id;
				reply->id = node_id;
				awake = false;
			}
		} else if (token.action == exec) {
			if (token.id == node_id) {
				char c = token.parent_id;
				if (c == SENTINEL) {
					if (flag_sentinel) {
						std::swap(text, pattern);
					} else {
						pthread_mutex_lock(&mutex);
						if (calc_queue.empty()) {
							pthread_cond_signal(&cond);
						}
						calc_queue.push({pattern, text});
						pthread_mutex_unlock(&mutex);
						text.clear();
						pattern.clear();
					}
					flag_sentinel = flag_sentinel ^ 1;
				} else {
					text = text + c;
				}
				reply->action = success;
			} else if (has_child) {
				node_token_t* token_down = new node_token_t(token);
				node_token_t reply_down(token);
				reply_down.action = fail;
				if (zmq_std::send_recieve_wait(token_down, reply_down, node_socket) and reply_down.action == success) {
					*reply = reply_down;
				}
			}
		}
		zmq_std::send_msg_dontwait(reply, node_parent_socket);
	}
	if (has_child) {
		rc = zmq_close(node_socket);
		assert(rc == 0);
		rc = zmq_ctx_term(node_context);
		assert(rc == 0);
	}
	rc = zmq_close(node_parent_socket);
	assert(rc == 0);
	rc = zmq_ctx_term(node_parent_context);
	assert(rc == 0);

	pthread_mutex_lock(&mutex);
	if (calc_queue.empty()) {
		pthread_cond_signal(&cond);
	}
	calc_queue.push({SENTINEL_STR, SENTINEL_STR});
	pthread_mutex_unlock(&mutex);

	rc = pthread_join(calculation_thread, NULL);
	assert(rc == 0);

	rc = pthread_cond_destroy(&cond);
	assert(rc == 0);
	rc = pthread_mutex_destroy(&mutex);
	assert(rc == 0);
}