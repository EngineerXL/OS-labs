#include "calculation_node.h"
#include <iostream>
#include <list>
#include <stdio.h>
#include <unistd.h>
#include <zmq.h>

#include <string.h>
#include <errno.h>

const char* SHARED_FILE_NAME = "texxxt.txt";

bool remove_id(std::list< std::list<long long> > & ids, const long long & id) {
	// std::cout << "Removing... " << id << std::endl;
	for (auto it = ids.begin(); it != ids.end(); ++it) {
		for (auto itt = it->begin(); itt != it->end(); ++itt) {
			if (id == *itt) {
				// std::cout << "it->size() = " << it->size() << std::endl;
				if (it->size() > 1) {
					it->erase(itt);
				} else {
					ids.erase(it);
				}
				return true;
			}
		}
	}
	return false;
}

int main() {

	void* context = zmq_ctx_new();
	void* socket = zmq_socket(context, ZMQ_REQ);
	zmq_bind(socket, "tcp://*:4080");

	void* text_context = zmq_ctx_new();
	void* text_socket = zmq_socket(text_context, ZMQ_PUB);
	zmq_bind(text_socket, "tcp://*:1234");

	std::list< std::list<long long> > ids;

	int cur_id = 1;

	int n;
	std::cin >> n;

	std::string s;
	while (cur_id > 0 and n-- and std::cin >> s) {
		std::cout << "-----------------> " << getpid() << " Token: " << s  << std::endl;
		if (s == "create") {
			long long id, parent;
			std::cin >> id >> parent;
			std::cout << "-----------------> " << getpid() << " Parent is " << parent << " id is " << id << std::endl;
			if (parent == -1) {
				std::list<long long> new_list;
				new_list.push_back(id);
				ids.push_back(new_list);

				cur_id = fork();
				std::cout << "My cur_id is" << cur_id << std::endl;
				if (cur_id == 0) {
					// creating new calculation node
					calculation_node_t new_node(id);
					int ret = new_node.execute();
					std::cout << getpid() << " with id [" << id << "] exited with return code " << ret << std::endl;
					return 0;
				} else {
					sleep(1);
				}
			} else {
				auto iter = ids.end();
				bool exists = false;
				for (auto it = ids.begin(); it != ids.end(); ++it) {
					for (auto itt = it->begin(); itt != it->end(); ++itt) {
						if (*itt == id) {
							exists = true;
							break;
						}
						if (*itt == parent) {
							iter = it;
						}
					}
				}
				if ((iter != ids.end()) & (!exists)) {
					iter->push_back(id);

					cur_id = fork();
					std::cout << "My cur_id is" << cur_id << std::endl;
					if (cur_id == 0) {
						// creating new calculation node
						calculation_node_t new_node(id);
						int ret = new_node.execute();
						std::cout << getpid() << " with id [" << id << "] exited with return code " << ret << std::endl;
						return 0;
					} else {
						sleep(1);
					}
				} else {
					if (exists) {
						std::cout << "Error: Already exists" << std::endl;
					} else if (iter == ids.end()) {
						std::cout << "Error: Parent not found" << std::endl;
					}
				}
			}
			
		} else if (s == "remove") {
			long long id;
			std::cin >> id;
			bool flag_available = false;
			for (auto it = ids.begin(); it != ids.end(); ++it) {
				for (auto itt = it->begin(); itt != it->end(); ++itt) {
					node_token_t* token = new node_token_t;
					token->action = destroy;
					token->id = id;

					// std::cout << "Cur id " << token->id << std::endl;

					int rc;

					zmq_msg_t message;
					zmq_msg_init(&message);
					rc = zmq_msg_init_size(&message, sizeof(node_token_t));
					assert(rc == 0);
					// memcpy(zmq_msg_data(&message), &token, sizeof(node_token_t));
					rc = zmq_msg_init_data(&message, token, sizeof(node_token_t), NULL, NULL);
					assert(rc == 0);
					// std::cout << "I'm here 222" << std::endl;
					rc = zmq_msg_send(&message, socket, ZMQ_DONTWAIT);
					if (rc == -1) {
						std::cout << "ERROR: " << strerror(errno) << std::endl;
						continue;
					}
					assert(rc == sizeof(node_token_t));
					zmq_msg_close(&message);

					zmq_msg_t reply;
					zmq_msg_init(&reply);
					std::cout << "I'm here 333" << std::endl;
					rc = zmq_msg_recv(&reply, socket, 0);
					assert(rc == sizeof(node_token_t));
					std::cout << "I'm here 444" << std::endl;
					node_token_t* token_reply = (node_token_t*)zmq_msg_data(&reply);
					if (token_reply->action == idle) {
						flag_available = true;
					}
					zmq_msg_close(&reply);
				}
			}

			bool flag_erased = remove_id(ids, id);
			if (flag_erased) {
				if (flag_available) {
					std::cout << "OK" << std::endl;
				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		} else if (s == "ping") {
			long long id;
			std::cin >> id;
			bool flag_available = false;
			bool flag_found = false;
			for (auto it = ids.begin(); it != ids.end(); ++it) {
				for (auto itt = it->begin(); itt != it->end(); ++itt) {
					node_token_t* token = new node_token_t;
					token->action = ping;
					token->id = id;

					// std::cout << "Cur id " << token->id << std::endl;

					int rc;

					zmq_msg_t message;
					zmq_msg_init(&message);
					rc = zmq_msg_init_size(&message, sizeof(node_token_t));
					assert(rc == 0);
					// memcpy(zmq_msg_data(&message), &token, sizeof(node_token_t));
					rc = zmq_msg_init_data(&message, token, sizeof(node_token_t), NULL, NULL);
					assert(rc == 0);
					// std::cout << "I'm here 222" << std::endl;
					rc = zmq_msg_send(&message, socket, ZMQ_DONTWAIT);
					if (rc == -1) {
						std::cout << "ERROR: " << strerror(errno) << std::endl;
						continue;
					}

					assert(rc == sizeof(node_token_t));
					zmq_msg_close(&message);

					zmq_msg_t reply;
					zmq_msg_init(&reply);
					// std::cout << "I'm here 333" << std::endl;
					rc = 	zmq_msg_recv(&reply, socket, 0);
					assert(rc == sizeof(node_token_t));
					// std::cout << "I'm here 444" << std::endl;
					node_token_t* token_reply = (node_token_t*)zmq_msg_data(&reply);
					if (*itt == id) {
						flag_found = true;
					}
					if (token_reply->action == idle) {
						flag_available = true;
					}
					zmq_msg_close(&reply);
				}
			}
			if (flag_found) {
				if (flag_available) {
					std::cout << "OK: 1" << std::endl;
				} else {
					std::cout << "OK: 0" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}

		} else if (s == "exec") {
			long long id;
			std::cin >> id;
			std::string pattern, text;
			std::cin >> pattern >> text;
			pattern += '$';
			text += '$';
			std::cout << "        TOKEN " << pattern << " " << text << std::endl;
			std::cout << "---------------------------------- I'm here!!!" << std::endl;
			int rc;

					//
					for (size_t i = 0; i < pattern.size(); ++i) {
						zmq_msg_t message;
						zmq_msg_init(&message);
						rc = zmq_msg_init_size(&message, sizeof(char));
						assert(rc == 0);
						rc = zmq_msg_init_data(&message, &pattern[i], sizeof(char), NULL, NULL);
						assert(rc == 0);
						rc = zmq_msg_send(&message, text_socket, 0);
						if (rc == -1) {
							std::cout << "ERROR: " << strerror(errno) << std::endl;
							continue;
						}
						assert(rc == sizeof(char));
						zmq_msg_close(&message);
					}
					//
					for (size_t i = 0; i < text.size(); ++i) {
						zmq_msg_t message;
						zmq_msg_init(&message);
						rc = zmq_msg_init_size(&message, sizeof(char));
						assert(rc == 0);
						rc = zmq_msg_init_data(&message, &text[i], sizeof(char), NULL, NULL);
						assert(rc == 0);
						rc = zmq_msg_send(&message, text_socket, 0);
						if (rc == -1) {
							std::cout << "ERROR: " << strerror(errno) << std::endl;
							continue;
						}
						assert(rc == sizeof(char));
						zmq_msg_close(&message);
					}


			bool flag_available = false;
			bool flag_found = false;
			for (auto it = ids.begin(); it != ids.end(); ++it) {
				for (auto itt = it->begin(); itt != it->end(); ++itt) {
					node_token_t* token = new node_token_t;
					token->action = calculate;
					token->id = id;
					// token->pattern = (char*)pattern.c_str();
					// token->text = (char*)text.c_str();

					// std::cout << "Cur id " << token->id << std::endl;

					int rc;

					zmq_msg_t message;
					zmq_msg_init(&message);
					rc = zmq_msg_init_size(&message, sizeof(node_token_t));
					assert(rc == 0);
					// memcpy(zmq_msg_data(&message), &token, sizeof(node_token_t));
					rc = zmq_msg_init_data(&message, token, sizeof(node_token_t), NULL, NULL);
					assert(rc == 0);
					// std::cout << "I'm here 222" << std::endl;
					rc = zmq_msg_send(&message, socket, ZMQ_DONTWAIT);
					if (rc == -1) {
						std::cout << "ERROR: " << strerror(errno) << std::endl;
						continue;
					}
					assert(rc == sizeof(node_token_t));
					zmq_msg_close(&message);
					zmq_msg_t reply;
					zmq_msg_init(&reply);
					std::cout << "I'm here 333" << std::endl;
					rc = zmq_msg_recv(&reply, socket, 0);
					assert(rc == sizeof(node_token_t));
					std::cout << "I'm here 444" << std::endl;
					node_token_t* token_reply = (node_token_t*)zmq_msg_data(&reply);
					if (*itt == id) {
						flag_found = true;
					}
					if (token_reply->action == idle) {
						flag_available = true;
						break;
					}
					zmq_msg_close(&reply);
				}
			}
			if (flag_found) {
				if (flag_available) {
					std::cout << "OK: 1" << std::endl;
				} else {
					std::cout << "OK: 0" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}

		}

		std::cout << "-----------------> " << getpid() << " Printing list..." << std::endl;
		for (auto it : ids) {
			for (auto itt : it) {
				std::cout << itt << " ";
			}
			std::cout << std::endl;
		}
		sleep(1);
	}

	if (cur_id > 0) {
		int rc;
		rc = zmq_close(text_socket);
		assert(rc == 0);
		rc = zmq_ctx_term(text_context);
		assert(rc == 0);

		rc = zmq_close(socket);
		assert(rc == 0);
		rc = zmq_ctx_term(context);
		assert(rc == 0);
	}
}
