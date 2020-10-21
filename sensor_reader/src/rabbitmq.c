#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <amqp_tcp_socket.h>

#include <rabbitmq.h>
#include <pthread.h>

amqp_connection_state_t conn;
amqp_basic_properties_t props;

char *hostname, *username, *password;
int port;

void rabbitmq_set_connection_params(const char* hostname_, const char* username_, const char* password_, int port_) {
	hostname = (char*)malloc(sizeof(char)*strlen(hostname_)+1);
	strcpy(hostname, hostname_);

	username = (char*)malloc(sizeof(char)*strlen(username_)+1);
	strcpy(username, username_);

	password = (char*)malloc(sizeof(char)*strlen(password_)+1);
	strcpy(password, password_);

	port = port_;
}

int rabbitmq_init() {
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

	amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, username, password);
	amqp_channel_open(conn, 1);
	amqp_get_rpc_reply(conn);

	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */

	return 0;
}

void send_message(char* message, char* exchange, char* routingkey) {
	int status = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
							  amqp_cstring_bytes(routingkey), 0, 0,
							  &props, amqp_cstring_bytes(message));

	if (status != AMQP_STATUS_OK) {
		rabbitmq_init();
		send_message(message, exchange, routingkey);
		printf("CONNECTION RECOVERED!\n");
	}
}

void close_connection() {
	amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn);
}