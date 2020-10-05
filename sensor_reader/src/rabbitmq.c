#include <unistd.h>
#include <stdio.h>

#include <amqp_tcp_socket.h>

#include <rabbitmq.h>

amqp_connection_state_t conn;
amqp_basic_properties_t props;

int rabbitmq_init(const char* hostname, const char* username, const char* password, int port) {
	int status;

	conn = amqp_new_connection();

	amqp_socket_t *socket = NULL;
	socket = amqp_tcp_socket_new(conn);
	if (!socket) {
		printf("Problem creating TCP socket\n");
		//return 1;
	}

	status = amqp_socket_open(socket, hostname, port);
	if (status) {
		printf("Problem opening TCP socket\n");
	}

	amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username, password);
	amqp_channel_open(conn, 1);
	amqp_get_rpc_reply(conn);

	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */

	return 0;
}

void send_message(char* message, char* exchange, char* routingkey) {
	amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
					   amqp_cstring_bytes(routingkey), 0, 0,
					   &props, amqp_cstring_bytes(message));
}

void close_connection() {
	amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn);
}

