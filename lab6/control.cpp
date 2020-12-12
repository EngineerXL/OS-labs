#include <unistd.h>

#include "topology.hpp"
#include "zmq_std.hpp"

const char SENTINEL = '$';
const char* NODE_EXECUTABLE_NAME = "calculation";

int main() {
	int rc;

	void* context = zmq_ctx_new();
	void* socket = zmq_socket(context, ZMQ_REQ);
	rc = zmq_bind(socket, "tcp://*:4080");
	assert(rc == 0);

	void* text_context = zmq_ctx_new();
	void* text_socket = zmq_socket(text_context, ZMQ_PUB);
	rc = zmq_bind(text_socket, "tcp://*:1234");
	assert(rc == 0);

	topology_t<long long> control_node;

	int cur_id = 1;

	std::string s;
	long long id;
	while (std::cin >> s >> id) {
		if (s == "create") {
			long long parent;
			std::cin >> parent;
			if (control_node.find(id)) {
				std::cout << "Error: Already exists" << std::endl;
			} else {
				bool inserted = false;
				if (parent == -1) {
					control_node.insert(id);
					inserted = true;
				} else {
					inserted = control_node.insert(parent, id);
				}
				if (inserted) {
					int fork_id = fork();
					if (fork_id == 0) {
						rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(id).c_str(), NULL);
						assert(rc != -1);
						return 0;
					}
				} else {
					std::cout << "Error: Parent not found" << std::endl;
				}
			}
		} else if (s == "remove") {
			if (control_node.erase(id)) {
				bool flag_removed = false;
				for (size_t i = 0; i < control_node.size() + 1; ++i) {
					node_token_t* token = new node_token_t({destroy, id});
					if (!zmq_std::send_msg_dontwait(token, socket)) {
						continue;
					}
					node_token_t token_reply = zmq_std::recieve_msg<node_token_t>(socket);
					if (token_reply.action == success and token_reply.id == id) {
						flag_removed = true;
						break;
					}
				}
				if (flag_removed) {
					std::cout << "OK" << std::endl;
				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		}  else if (s == "ping") {
			if (control_node.find(id)) {
				bool flag_available = false;
				for (size_t i = 0; i < control_node.size(); ++i) {
					node_token_t* token = new node_token_t({ping, id});
					if (!zmq_std::send_msg_dontwait(token, socket)) {
						continue;
					}
					node_token_t token_reply = zmq_std::recieve_msg<node_token_t>(socket);
					if (token_reply.action == success and token_reply.id == id) {
						flag_available = true;
						break;
					}
				}
				if (flag_available) {
					std::cout << "OK: 1" << std::endl;
				} else {
					std::cout << "OK: 0" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		} else if (s == "exec") {
			std::string pattern, text;
			std::cin >> pattern >> text;
			if (control_node.find(id)) {
				bool flag_available = false;
				for (size_t i = 0; i < control_node.size(); ++i) {
					node_token_t* token = new node_token_t({ping, id});
					if (!zmq_std::send_msg_dontwait(token, socket)) {
						continue;
					}
					node_token_t token_reply = zmq_std::recieve_msg<node_token_t>(socket);
					if (token_reply.action == success and token_reply.id == id) {
						flag_available = true;
						break;
					}
				}
				if (flag_available) {
					std::string text_pattern = pattern + SENTINEL + text + SENTINEL;
					for (size_t i = 0; i < text_pattern.size(); ++i) {
						char* c = new char(text_pattern[i]);
						zmq_std::send_msg(c, text_socket);
					}
					for (size_t i = 0; i < control_node.size(); ++i) {
						node_token_t* token = new node_token_t({calculate, id});
						if (!zmq_std::send_msg_dontwait(token, socket)) {
							continue;
						}
						node_token_t token_reply = zmq_std::recieve_msg<node_token_t>(socket);
						if (token_reply.action == success and token_reply.id == id) {
							flag_available = true;
							break;
						}
					}
				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		}
		sleep(1);
	}

	if (cur_id > 0) {
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