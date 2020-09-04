//gcc -Ilibrabbitmq -Llibrabbitmq test_send.c -lrabbitmq

#include <stdio.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

int main() {
	char const *hostname = "mekosoft.vn";
	int port = 5672, status;
	char const *exchange = "ex_sensors";
	char const *routingkey = "event.pir.haha";
	char const *messagebody = "haha";
	amqp_socket_t *socket = NULL;
	amqp_connection_state_t conn;

	conn = amqp_new_connection();
	socket = amqp_tcp_socket_new(conn);
	status = amqp_socket_open(socket, hostname, port);

	amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,"admin", "admin");
	amqp_channel_open(conn, 1);
	amqp_get_rpc_reply(conn);


	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
					   amqp_cstring_bytes(routingkey), 0, 0,
					   &props, amqp_cstring_bytes(messagebody));

	amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn);

	return 0;
}