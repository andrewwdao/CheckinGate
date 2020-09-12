#ifndef MAIN_
#define MAIN_

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>

// Hard coded for testing
char* exchange_name = "ex_sensors";
char* routing_key_prefix = "event";
char* rabbitmq_host = "demo1.gate.mekosoft.vn";
char* rabbitmq_username = "admin";
char* rabbitmq_password = "admin";
int rabbitmq_port = 5672;

uint8_t run_rabbitmq = 1;
uint8_t run_pir = 1;
uint8_t run_rfid = 0;
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

// PIR flags
#define PIR_CNT 4
uint8_t pir_flags[PIR_CNT+1] = {0,0,0,0,0};
uint8_t pir_will_send[PIR_CNT+1] = {1,1,1,1,1};
pthread_t pir_thread_id[PIR_CNT+1];


char* format_message(char* sensor, char* src, char* data) {
	char* message = (char*)malloc(300);
	
	struct timeb ts;
	ftime(&ts);

	snprintf(message, 300,
		"{"
			"\"timestamp\":%llu,"
			"\"event_type\":\"%s\","
			"\"source\":\"%s\","
			"\"data\":\"%s\""
		"}",
		(unsigned long long int)ts.time*1000ll + ts.millitm, sensor, src, data
	);

	return message;
}

void* pir_delay_handler(void* id_ptr) {
	uint8_t id = *(uint8_t*)id_ptr;

	pir_will_send[id] = 0;

	usleep(200000);
	
	pir_will_send[id] = 1;
}

void pir_send(uint8_t id) {
	pir_flags[id] = 0;

	pthread_create(&pir_thread_id[id], NULL, pir_delay_handler, &id);

	char pir_src[10];
	snprintf(pir_src, 10, "pir.%d", id);
	char routing_key[30];
	snprintf(routing_key, 20, "%s.%s", routing_key_prefix, pir_src);
	char data[20];
	snprintf(data, 10, "pir_id:%d", id);

	if (run_rabbitmq) {
		send_message(format_message("pir", pir_src, data),
				 	 exchange_name, routing_key);
	}
	
}

void pir_isr_handler(uint8_t id) {
	printf("PIR: %d\n", id);

	if (pir_will_send[id])
		pir_flags[id] = 1;
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
				 exchange_name, routing_key);
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
				 exchange_name, routing_key);
}

int main() {
	if (run_rabbitmq) {
		printf("Init rabbitmq...\n");
		rabbitmq_init(rabbitmq_host, rabbitmq_username, rabbitmq_password, rabbitmq_port);
	}

	if (run_pir) {
		printf("Init PIR...\n");

		pir_set_ext_isr(pir_isr_handler);
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
	else while(1) {
		for (uint8_t i = 1; i <= PIR_CNT; i++) {
			if (pir_flags[i] && pir_will_send[i]) pir_send(i);
		}
	}

	return 0;
}

#endif