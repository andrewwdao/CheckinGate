#include <unistd.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <malloc.h>
#include <string.h>

#include <pthread.h>

#include <amqp_tcp_socket.h>

#include <rabbitmq.h>
#include <sensor_reader.h>

#define CONNECTION_COUNT 1

amqp_connection_state_t conn[CONNECTION_COUNT];
amqp_basic_properties_t props;

char *hostname, *username, *password;
int port;
int current_connection = 0;

/**
 *  @brief Get current system time
 *  @return system time in millisecond
 */
uint64_t get_current_time(void)
{
	struct timeb ts;
	ftime(&ts);
	return (uint64_t)ts.time*1000ll + ts.millitm;
}


/**
 *  @brief Format recieved message to established standard to send to RabbitMQ
 *  @param sensor type of sensor
 *  @param src source of trigger
 *  @param data data to send
 *  @return formatted string
 */
char* format_message(uint64_t timestamp, char* sensor, char* src, char* data, uint8_t sensor_id)
{
	char* message = (char*)malloc(300);

	snprintf(message, 300,
		"{"
			"\"timestamp\":%llu,"
			"\"event_type\":\"%s\","
			"\"source\":\"%s\","
			"\"data\":\"%s\""
		"}",
		timestamp, sensor, src, data
	);
	//printf("%s\n", message);
	return message;
}

void rabbitmq_set_connection_params(const char* hostname_, const char* username_, const char* password_, int port_) {
	hostname = (char*)malloc(sizeof(char)*strlen(hostname_)+1);
	strcpy(hostname, hostname_);

	username = (char*)malloc(sizeof(char)*strlen(username_)+1);
	strcpy(username, username_);

	password = (char*)malloc(sizeof(char)*strlen(password_)+1);
	strcpy(password, password_);

	port = port_;
}

void rabbitmq_init_with_id(int id) {
	int status;

	conn[id] = amqp_new_connection();

	amqp_socket_t *socket = NULL;
	socket = amqp_tcp_socket_new(conn[id]);
	if (!socket) {
		printf("Problem creating TCP socket\n");
		//return 1;
	}

	status = amqp_socket_open(socket, hostname, port);
	if (status) {
		printf("Problem opening TCP socket\n");
	}

	amqp_login(conn[id], "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username, password);
	amqp_channel_open(conn[id], 1);
	amqp_get_rpc_reply(conn[id]);

	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
}

int rabbitmq_init() {
	for (int i = 0; i < CONNECTION_COUNT; ++i) {
		rabbitmq_init_with_id(i);
	}

	return 0;
}

void send_message(char* message, char* exchange, char* routingkey) {
	int status = amqp_basic_publish(conn[current_connection], 1, amqp_cstring_bytes(exchange),
							  amqp_cstring_bytes(routingkey), 0, 0,
							  &props, amqp_cstring_bytes(message));

	if (status != AMQP_STATUS_OK) {
		rabbitmq_init_with_id(current_connection);
		current_connection = (current_connection + 1)%CONNECTION_COUNT;
		send_message(message, exchange, routingkey);
		printf("CONNECTION RECOVERED!\n");
	}
}

void close_connection() {
	amqp_channel_close(conn[current_connection], 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn[current_connection], AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn[current_connection]);
}
