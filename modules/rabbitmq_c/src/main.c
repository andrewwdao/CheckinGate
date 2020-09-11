#ifndef MAIN_
#define MAIN_

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>

amqp_connection_state_t conn;
amqp_basic_properties_t props;

// Hard coded for testing
char* exchange_name = "ex_sensors";
char* routing_key_prefix = "event";
char* rabbitmq_host = "localhost";
char* rabbitmq_username = "admin";
char* rabbitmq_password = "admin";
int rabbitmq_port = 5672;

uint8_t run_rabbitmq = 1;
uint8_t run_pir = 1;
uint8_t run_rfid = 1;
uint8_t run_uhf = 0;

uint8_t pir_1_pin = 1;
uint8_t pir_2_pin = 4;
uint8_t pir_3_pin = 5;
uint8_t pir_4_pin = 6;

uint8_t rfid_1_d0_pin = 2;
uint8_t rfid_1_d1_pin = 3;
uint8_t rfid_2_d0_pin = 7;
uint8_t rfid_2_d1_pin = 0;
uint8_t oe_pin = 11;
const char* uhf_port = "/dev/serial0";

char* format_message(char* sensor, char* src, char* data) {
	char* message = (char*)malloc(300);

	snprintf(message, 300,
		"{"
			"\"timestamp\":%llu,"
			"\"event_type\":\"%s\","
			"\"source\":\"%s\","
			"\"data\":\"%s\""
		"}",
		(unsigned long long)time(NULL), sensor, src, data
	);

	return message;
}

void pir_isr_handler(char* id) {
	printf("PIR: %s\n", id);
	fflush(stdout);

	char pir_src[10];
	snprintf(pir_src, 10, "pir.%s", id);
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", routing_key_prefix, pir_src);
	char data[10];
	snprintf(data, 10, "pir_id:%s", id);

	if (run_rabbitmq)
	send_message(format_message("pir", pir_src, data),
				 exchange_name, routing_key,
				 conn, &props);
}

void rfid_timeout_handler(uint8_t id, uint32_t fullcode) {
	if (!fullcode) {
		printf("RFID %d: CHECKSUM FAILED\n", id);
		return;
	}

	printf("RFID %d: 0x%06X\n", id, fullcode);
	fflush(stdout);

	char rfid_src[10];
	snprintf(rfid_src, 10, "rfid.%d", id);
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", routing_key_prefix, rfid_src);
	char data[20];
	snprintf(data, 20, "tag_id:0x%06X", fullcode);

	if (run_rabbitmq)
	send_message(format_message("rfid", rfid_src, data),
				 exchange_name, routing_key,
				 conn, &props);
}

void uhf_read_handler(char* read_data) {
	if (read_data[0] == 'E' && read_data[1] == 'R' &&
		read_data[2] == 'R' && read_data[3] == '\0') {
		return;
	}

	char* uhf_src = "uhf.1";
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", routing_key_prefix, uhf_src);
	char data[150];
	snprintf(data, 150, "tag_id:0x%s", read_data);

	if (run_rabbitmq)
	send_message(format_message("uhf", uhf_src, data),
				 exchange_name, routing_key,
				 conn, &props);
}

int main() {
	if (run_rabbitmq) {
		printf("Init rabbitmq...\n");
		rabbitmq_init(rabbitmq_host, rabbitmq_username, rabbitmq_password, rabbitmq_port, &conn, &props);
	}

	if (run_pir) {
		printf("Init PIR...\n");
		pir_set_ext_isr_handler(&pir_isr_handler);
		pir_init(pir_1_pin, pir_2_pin, pir_3_pin, pir_4_pin);
	}

	if (run_rfid) {
		printf("Init RFID...\n");

		int rfid_id = 0;

		void rfid_1_d0_isr() { handle_isr(rfid_id, RFID_D0_BIT); }
		void rfid_1_d1_isr() { handle_isr(rfid_id, RFID_D1_BIT); }
		rfid_init(rfid_1_d0_pin, rfid_1_d1_pin, 0, rfid_1_d0_isr, rfid_1_d1_isr, rfid_timeout_handler);
		
		++rfid_id;

		void rfid_2_d0_isr() { handle_isr(rfid_id, RFID_D0_BIT); }
		void rfid_2_d1_isr() { handle_isr(rfid_id, RFID_D1_BIT); }
		rfid_init(rfid_2_d0_pin, rfid_2_d1_pin, oe_pin, rfid_2_d0_isr, rfid_2_d1_isr, rfid_timeout_handler);
	}

	if (run_uhf) {
		printf("Init UHF RFID...\n");
		uhf_set_param(0x02, 0x01, 11);
		uhf_init(uhf_port, 115200, 11);
	}

	printf("System running...\n");
	fflush(stdout);
	
	if (run_uhf) while(1) uhf_read_handler(uhf_realtime_inventory());
	else while(1) pause();

	return 0;
}

#endif