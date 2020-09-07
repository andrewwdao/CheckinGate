#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>

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
	printf("PIR: 1\n");
	fflush(stdout);
}
void pir_2_isr() {
	printf("PIR: 2\n");
	fflush(stdout);
}
void pir_3_isr() {
	printf("PIR: 3\n");
	fflush(stdout);
}
void pir_4_isr() {
	printf("PIR: 4\n");
	fflush(stdout);
}

void rfid_timeout_handler(int full_code) {
	printf("RFID: 0x%X\n", full_code);
	fflush(stdout);
}

void uhf_read_handler(int data) {
	printf("UHF RFID: 0x%s\n");
}

int main() {
	uint8_t run_pir = 1;
	uint8_t run_rfid = 1;
	uint8_t run_uhf = 0;


	amqp_connection_state_t conn;
	amqp_basic_properties_t props;

	rabbitmq_init("mekosoft.vn", "admin", "admin", 5672,&conn,&props);
	send_message(format_message("pir", 1, "1"),
				 "ex_sensors", "event.pir.haha",
				 conn, &props);

	if (run_pir) {
		printf("Init PIR...\n");
		set_pir_isr_handler(1, &pir_1_isr);
		set_pir_isr_handler(2, &pir_2_isr);
		set_pir_isr_handler(3, &pir_3_isr);
		set_pir_isr_handler(4, &pir_4_isr);
		pir_init(0,0,0,0);
	}

	if (run_rfid) {
		printf("Init RFID...\n");
		set_rfid_ext_timeout_handler(&rfid_timeout_handler);
		rfid_init(0,0,0);
	}

	if (run_uhf) {
		printf("Init UHF RFID...\n");
		uhf_set_param(0x02, 0x01, 11);
		uhf_init("PORT", 115200, 11);
	}

	if (run_uhf) while(1) uhf_read_tag();
	else while(1) pause();

	return 0;
}