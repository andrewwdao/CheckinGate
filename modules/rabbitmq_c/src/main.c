#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>

uint8_t run_pir = 1;
uint8_t run_rfid = 1;
uint8_t run_uhf = 1;

char* format_message(char* sensor, uint8_t sensor_id, char* raw_data) {
	char* message = malloc(200);

	sprintf(message,
		"{"
			"\"timestamp\":%llu,"
			"\"event_type\":\"%s\","
			"\"source\":\"%s.%d\","
			"\"data\":\"%s\""
		"}",
		(unsigned long long)time(NULL), sensor, sensor, (int)sensor_id, raw_data
	);

	return message;
}

void pir_1_isr() {
	printf("c1\n");
	fflush(stdout);
}
void pir_2_isr() {
	printf("c2\n");
	fflush(stdout);
}
void pir_3_isr() {
	printf("c3\n");
	fflush(stdout);
}
void pir_4_isr() {
	printf("c4\n");
	fflush(stdout);
}

void rfid_timeout_handler(int full_code) {
	printf("%d", full_code);
	fflush(stdout);
}

int main() {
	amqp_connection_state_t conn;
	amqp_basic_properties_t props;

	rabbitmq_init("mekosoft.vn", "admin", "admin", 5672,&conn,&props);
	send_message(format_message("pir", 1, "1"),
				 "ex_sensors", "event.pir.haha",
				 conn, &props);

	if (run_pir) {
		set_pir_isr_handler(1, &pir_1_isr);
		set_pir_isr_handler(2, &pir_2_isr);
		set_pir_isr_handler(3, &pir_3_isr);
		set_pir_isr_handler(4, &pir_4_isr);
		pir_init(0,0,0,0);
	}

	if (run_rfid) {
		set_ext_timeout_handler(&rfid_timeout_handler);
		pir_init(0,0,0,0);
	}

	return 0;
}