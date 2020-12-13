#include <unistd.h>
#include <vector>

#include "topology.hpp"
#include "zmq_std.hpp"

using node_id_type = long long;

const long long TEXT_PORT = 100;

int main() {
	int rc;
	topology_t<node_id_type> control_node;
	std::vector< std::pair<void*, void*> > childs;

	std::string s;
	node_id_type id;
	while (std::cin >> s >> id) {
		if (s == "create") {
			node_id_type parent_id;
			std::cin >> parent_id;
			if (parent_id == -1) {
				void* new_context = NULL;
				void* new_socket = NULL;
				zmq_std::init_pair_socket(new_context, new_socket);
				rc = zmq_bind(new_socket, ("tcp://*:" + std::to_string(PORT_BASE + id)).c_str());
				assert(rc == 0);

				int fork_id = fork();
				if (fork_id == 0) {
					rc = execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(id).c_str(), NULL);
					assert(rc != -1);
					return 0;
				} else {
					node_token_t* token = new node_token_t({ping, id, id});
					node_token_t reply({fail, id, id});
					if (zmq_std::send_recieve_wait(token, reply, new_socket) and reply.action == success) {
						childs.push_back(std::make_pair(new_context, new_socket));
						control_node.insert(id);
					} else {
						rc = zmq_close(new_socket);
						assert(rc == 0);
						rc = zmq_ctx_term(new_context);
						assert(rc == 0);
					}
				}
			} else if (control_node.find(parent_id) == -1) {
				std::cout << "Error: Not found" << std::endl;
			} else {
				if (control_node.find(id) != -1) {
					std::cout << "Error: Already exists" << std::endl;
				} else {
					int ind = control_node.find(parent_id);
					node_token_t* token = new node_token_t({create, parent_id, id});
					node_token_t reply({fail, id, id});
					if (zmq_std::send_recieve_wait(token, reply, childs[ind].second) and reply.action == success) {
						control_node.insert(parent_id, id);
					} else {
						std::cout << "Error: Parent is unavailable" << std::endl;
					}
				}
			}
		} else if (s == "remove") {
			int ind = control_node.find(id);
			if (ind != -1) {
				node_token_t* token = new node_token_t({destroy, id, id});
				node_token_t reply({fail, id, id});
				bool ok = zmq_std::send_recieve_wait(token, reply, childs[ind].second);
				if (reply.action == destroy and reply.parent_id == id) {
					rc = zmq_close(childs[ind].second);
					assert(rc == 0);
					rc = zmq_ctx_term(childs[ind].first);
					assert(rc == 0);
					std::vector< std::pair<void*, void*> >::iterator it = childs.begin();
					while (ind--) {
						++it;
					}
					childs.erase(it);
				} else if (reply.action == bind and reply.parent_id == id) {
					rc = zmq_close(childs[ind].second);
					assert(rc == 0);
					rc = zmq_ctx_term(childs[ind].first);
					assert(rc == 0);
					zmq_std::init_pair_socket(childs[ind].first, childs[ind].second);
					rc = zmq_bind(childs[ind].second, ("tcp://*:" + std::to_string(PORT_BASE + reply.id)).c_str());
					assert(rc == 0);
				}
				if (ok) {
					control_node.erase(id);
					std::cout << "OK" << std::endl;
				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		} else if (s == "ping") {
			int ind = control_node.find(id);
			if (ind != -1) {
				node_token_t* token = new node_token_t({ping, id, id});
				node_token_t reply({fail, id, id});
				if (zmq_std::send_recieve_wait(token, reply, childs[ind].second) and reply.action == success) {
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
			int ind = control_node.find(id);
			if (ind != -1) {
				bool ok = true;
				std::string text_pattern = pattern + SENTINEL + text + SENTINEL;
				for (size_t i = 0; i < text_pattern.size(); ++i) {
					node_token_t* token = new node_token_t({exec, text_pattern[i], id});
					node_token_t reply({fail, id, id});
					if (!zmq_std::send_recieve_wait(token, reply, childs[ind].second) or reply.action != success) {
						ok = false;
						break;
					}
				}
				if (ok) {
					std::cout << "OK" << std::endl;
				} else {
					std::cout << "Error: Node is unavailable" << std::endl;
				}
			} else {
				std::cout << "Error: Not found" << std::endl;
			}
		}
	}

	for (size_t i = 0; i < childs.size(); ++i) {
		rc = zmq_close(childs[i].second);
		assert(rc == 0);
		rc = zmq_ctx_term(childs[i].first);
		assert(rc == 0);
	}
}