#ifndef ZMQ_STD_HPP
#define ZMQ_STD_HPP

#include <assert.h>
#include <zmq.h>

namespace zmq_std {
	template<class T>
	T recieve_msg(void* socket) {
		int rc = 0;
		zmq_msg_t reply;
		zmq_msg_init(&reply);
		rc = zmq_msg_recv(&reply, socket, 0);
		assert(rc == sizeof(T));
		T reply_data = *(T*)zmq_msg_data(&reply);
		rc = zmq_msg_close(&reply);
		assert(rc == 0);
		return reply_data;
	}

	/*
	 * This functions cause memory leakage because deleter is NULL
	 * Deleter must be thread safe to work properly
	 */
	template<class T>
	void send_msg(T* token, void* socket) {
		int rc = 0;
		zmq_msg_t message;
		zmq_msg_init(&message);
		rc = zmq_msg_init_size(&message, sizeof(T));
		assert(rc == 0);
		rc = zmq_msg_init_data(&message, token, sizeof(T), NULL, NULL);
		assert(rc == 0);
		rc = zmq_msg_send(&message, socket, 0);
		assert(rc == sizeof(T));
	}

	/* Returns true if T was successfully queued on the socket */
	template<class T>
	static bool send_msg_dontwait(T* token, void* socket) {
		int rc;
		zmq_msg_t message;
		zmq_msg_init(&message);
		rc = zmq_msg_init_size(&message, sizeof(T));
		assert(rc == 0);
		rc = zmq_msg_init_data(&message, token, sizeof(T), NULL, NULL);
		assert(rc == 0);
		rc = zmq_msg_send(&message, socket, ZMQ_DONTWAIT);
		/* DONTWAIT flag doesn't block process */
		if (rc == -1) {
			zmq_msg_close(&message);
			return false;
		}
		assert(rc == sizeof(T));
		return true;
	}
}

#endif /* ZMQ_STD_HPP */