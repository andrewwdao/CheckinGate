#ifndef MAIN_
#define MAIN_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>

#define MAIN_RFID_1 0 //index for rfid module 1
#define MAIN_RFID_2 1 //index for rfid module 2
// Hard coded for testing
#define EXCHANGE_NAME 		"ex_sensors"
#define ROUTING_KEY_PREFIX	"event"
#define HOST				"demo1.gate.mekosoft.vn"
#define USERNAME			"admin"
#define PASSWORD			"admin"
#define PORT 				5672

#define run_rabbitmq 1
#define run_pir		 1
#define run_rfid	 1
#define run_uhf		 1

#define PIR_1_PIN  4 //wiringpi pin
#define PIR_2_PIN  5 //wiringpi pin
#define PIR_3_PIN -1 //wiringpi pin
#define PIR_4_PIN -1 //wiringpi pin

#define RFID_1_D0_PIN 7 //wiringpi pin
#define RFID_1_D1_PIN 0 //wiringpi pin
#define RFID_2_D0_PIN 2 //wiringpi pin
#define RFID_2_D1_PIN 3 //wiringpi pin
#define OE_PIN 		  11 //wiringpi pin
#define UHF_PORT 	  "/dev/serial0"
#define UHF_BAUDRATE  115200
// --- PIR flags
#define PIR_CNT 4
#define PIR_DELAY 200000 //us
uint8_t pir_flags[PIR_CNT+1] = {0,0,0,0,0};
uint8_t pir_will_send[PIR_CNT+1] = {1,1,1,1,1};
pthread_t pir_thread_id;
// --- UHF
pthread_t uhf_thread_id;


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

void pir_send(uint8_t id) {
	pir_flags[id] = 0;
	pir_will_send[id] = 0;

	char pir_src[10];
	snprintf(pir_src, 10, "pir.%d", id);
	char routing_key[30];
	snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, pir_src);
	char data[20];
	snprintf(data, 10, "pir_id:%d", id);

	if (run_rabbitmq) {
		send_message(format_message("pir", pir_src, data),
				 	 EXCHANGE_NAME, routing_key);
	}

	// Remove these lines if threads are used
	usleep(PIR_DELAY);
	pir_will_send[id] = 1;
}

void* pir_loop(void* arg) {
	for (uint8_t i = 1; i <= PIR_CNT; i++) {
		if (pir_flags[i]) pir_send(i);
	}
}

void pir_isr_handler(uint8_t id) {
	printf("PIR: %d\n", id);

	if (pir_will_send[id]) {
		if (id == 1) system("./cap_108 &");
		else if (id == 2) system("./cap_109 &");

		pir_flags[id] = 1;
		pthread_create(&pir_thread_id, NULL, pir_loop, NULL);
	}
}

void rfid_timeout_handler(uint8_t id, uint32_t fullcode) {
	++id;

	if (!fullcode) {
		printf("RFID %d: CHECKSUM FAILED\n", id);
		return;
	}

	printf("RFID %d: 0x%06X\n", id, fullcode);
	fflush(stdout);

	char rfid_src[10];
	snprintf(rfid_src, 10, "rfid.%d", id);
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, rfid_src);
	char data[20];
	snprintf(data, 20, "tag_id:0x%06X", fullcode);

	if (run_rabbitmq)
	send_message(format_message("rfid", rfid_src, data),
				 EXCHANGE_NAME, routing_key);
}

void uhf_read_handler(char* read_data) {
	if (read_data[0] == 'E' && read_data[1] == 'R' &&
		read_data[2] == 'R' && read_data[3] == '\0') {
		return;
	}
	char* uhf_src = "rfid.3";
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, uhf_src);
	char data[150];
	snprintf(data, 150, "tag_id:0x%s", read_data);

	if (run_rabbitmq)
	send_message(format_message("rfid", uhf_src, data),
				 EXCHANGE_NAME, routing_key);
}

void* uhf_thread(void* arg) {
	while(1) uhf_read_handler(uhf_realtime_inventory());
	// uhf_read_tag();
	// while(1) uhf_read_handler(uhf_read_tag());
}

int main() {
	if (run_rabbitmq) {
		printf("Init rabbitmq...\n");
		rabbitmq_init(HOST, USERNAME, PASSWORD, PORT);
	}

	if (run_pir) {
		printf("Init PIR...\n");
		pir_set_ext_isr(pir_isr_handler);
		pir_init(PIR_1_PIN, PIR_2_PIN, PIR_3_PIN, PIR_4_PIN);
	}

	if (run_rfid) {
		printf("Init RFID...\n");
		void rfid_1_d0_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_1, RFID_D0_BIT)
		void rfid_1_d1_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_1, RFID_D1_BIT)
		rfid_init(MAIN_RFID_1, RFID_1_D0_PIN, RFID_1_D1_PIN, OE_PIN, rfid_1_d0_isr, rfid_1_d1_isr, rfid_timeout_handler);
		
		void rfid_2_d0_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_2, RFID_D0_BIT)
		void rfid_2_d1_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_2, RFID_D1_BIT)
		rfid_init(MAIN_RFID_2, RFID_2_D0_PIN, RFID_2_D1_PIN, RFID_NO_OE_PIN, rfid_2_d0_isr, rfid_2_d1_isr, rfid_timeout_handler);
	}

	if (run_uhf) {
		printf("Init UHF RFID...\n");
		uhf_set_param(EPC_MEMBANK, 0x01, 7);
		uhf_init(UHF_PORT, UHF_BAUDRATE, OE_PIN);
		pthread_create(&uhf_thread_id, NULL, uhf_thread, NULL);
	}

	printf("System running...\n");
	fflush(stdout);
	
	while(1) {
		pause();
	}
	return 0;
}

#endif