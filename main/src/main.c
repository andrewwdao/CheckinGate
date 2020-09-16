/** ------------------------------------------------------------*-
 * Main peripheral communication file.
 * CheckinGate Project.
 * Tested on Rasberry OS with RASPBERRY PI 4B.
 * (c) Anh-Khoi Tran 2020
 * (c) Minh-An Dao 2020

 * version 1.00 - 16/09/2020
 *--------------------------------------------------------------
 * @note If you cannot find any system configuration here
 * 		 Please find config.h to execute your changes.
 * 
 -------------------------------------------------------------- */
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


#define en_rabbitmq 1
#define en_pir		1
#define en_rfid	 	1
#define en_uhf		1
#define en_camera	1


// ------------------------- Constants -----------------------------------
// --- RabitMQ server infos
#define EXCHANGE_NAME 		"ex_sensors"
#define ROUTING_KEY_PREFIX	"event"
#define HOST				"demo1.gate.mekosoft.vn"
#define USERNAME			"admin"
#define PASSWORD			"admin"
#define PORT 				5672

// --- PIR parameters
#define PIR_CNT 4
#define PIR_DEBOUNCE 200000 //us
#define PIR_1_PIN  4 //wiringpi pin
#define PIR_2_PIN  5 //wiringpi pin
#define PIR_3_PIN -1 //wiringpi pin
#define PIR_4_PIN -1 //wiringpi pin

// --- RFID parameters
#define MAIN_RFID_1 0 //index for rfid module 1
#define MAIN_RFID_2 1 //index for rfid module 2
#define RFID_1_D0_PIN 7 //wiringpi pin
#define RFID_1_D1_PIN 0 //wiringpi pin
#define RFID_2_D0_PIN 2 //wiringpi pin
#define RFID_2_D1_PIN 3 //wiringpi pin
#define OE_PIN 		  11 //wiringpi pin

// --- UHF parameters
#define UHF_PORT 	  "/dev/serial0"
#define UHF_BAUDRATE  115200

// --- Camera parameter
#define IMAGE_DIR 	  "./img"
// ------------------------- Variables -----------------------------------
// --- PIR
uint8_t pir_flags[PIR_CNT+1] = {0,0,0,0,0};
uint8_t pir_will_send[PIR_CNT+1] = {1,1,1,1,1};
pthread_t pir_thread_id;
// --- UHF
pthread_t uhf_thread_id;
// --- Keep track of time
uint64_t now;

//--------------------------------------------------------------
// FUNCTION DEFINITIONS
//--------------------------------------------------------------
/**
 *  @brief first connection to the camera stream to make it ready
 */
void camera_init()
{
	char cmd[100];
	snprintf(cmd, 100, "(./src/cam01 %s 0 0 && ./src/cam02 %s 0 0 && rm -rf %s/*.jpg) &",IMAGE_DIR, IMAGE_DIR, IMAGE_DIR);
	system(cmd);
}

/**
 *  @brief Get current system time
 *  @return system time in millisecond
 */
uint64_t get_current_time()
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
char* format_message(char* sensor, char* src, char* data)
{
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
		now, sensor, src, data
	);

	return message;
}

/**
 *  @brief send pir interrupt signal to rabbitMQ
 *  @param arg id of the pir to be sent
 *  @return void*
 */
void* pir_send(void* arg) {
	uint8_t id = *(uint8_t*)arg;
	// pir_flags[id] = 0;
	pir_will_send[id] = 0;

	if (en_rabbitmq) {
		char pir_src[10];
		snprintf(pir_src, 10, "pir.%d", id);
		char routing_key[30];
		snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, pir_src);
		char data[20];
		snprintf(data, 10, "pir_id:%d", id);

		send_message(format_message("pir", pir_src, data),
				 	 EXCHANGE_NAME, routing_key);
	}

	// Remove these lines if threads are used
	usleep(PIR_DEBOUNCE);
	pir_will_send[id] = 1;
}

// void* pir_loop(void* arg) {
// 	for (uint8_t i = 1; i <= PIR_CNT; i++) {
// 		if (pir_flags[i]) pir_send(i);
// 	}
// }

/**
 *  @brief ISR handler for PIR sensors
 *  @param id id of the pir to be sent
 */
void pir_isr_handler(uint8_t id) {
	printf("PIR: %d\n", id);

	if (pir_will_send[id]) {
		now = get_current_time();
		// --- capture camera
		char cmd[100];
		snprintf(cmd, 100, "./src/cam0%d %s pir%d %llu", id, IMAGE_DIR, id, now);
		system(cmd);

		// pir_flags[id] = 1;
		pthread_create(&pir_thread_id, NULL, pir_send, &id);
	}
}

/**
 *  @brief timeout handler for wiegand26 protocol
 *  @param id id of the rfid being captured
 *  @param fullcode tag ID of the RFID
 */
void rfid_timeout_handler(uint8_t id, uint32_t fullcode) {

	if (!fullcode) {
		printf("RFID %d: CHECKSUM FAILED\n", ++id);
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

	now = get_current_time();

	if (en_rabbitmq)
	send_message(format_message("rfid", rfid_src, data),
				 EXCHANGE_NAME, routing_key);
}

/**
 *  @brief ISR handler for UHF RFID reader - RS232
 *  @param read_data data that the reader return
 */
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

	if (en_rabbitmq)
	send_message(format_message("rfid", uhf_src, data),
				 EXCHANGE_NAME, routing_key);
}

/**
 *  @brief UHF always-on thread to catch RS232 signal
 *  @note: This function will be replaced when wiegand26 protocol is applied for UHF reader
 *  @param arg void argument for the thread to be created
 *  @return void*
 */
void* uhf_thread(void* arg) {
	while(1) uhf_read_handler(uhf_realtime_inventory());
	// uhf_read_tag();
	// while(1) uhf_read_handler(uhf_read_tag());
}

int main() {

	if (en_rabbitmq) {
		printf("Init RabbitMQ...\n");
		rabbitmq_init(HOST, USERNAME, PASSWORD, PORT);
	}

	if (en_pir) {
		printf("Init PIRs...\n");
		pir_set_ext_isr(pir_isr_handler);
		pir_init(PIR_1_PIN, PIR_2_PIN, PIR_3_PIN, PIR_4_PIN);
	}

	if (en_rfid) {
		printf("Init RFIDs...\n");
		// --- RFID 1
		void rfid_1_d0_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_1, RFID_D0_BIT)
		void rfid_1_d1_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_1, RFID_D1_BIT)
		rfid_init(MAIN_RFID_1, RFID_1_D0_PIN, RFID_1_D1_PIN, OE_PIN, rfid_1_d0_isr, rfid_1_d1_isr, rfid_timeout_handler);
		// --- RFID 2
		void rfid_2_d0_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_2, RFID_D0_BIT)
		void rfid_2_d1_isr RFID_CREATE_ISR_HANDLER(MAIN_RFID_2, RFID_D1_BIT)
		rfid_init(MAIN_RFID_2, RFID_2_D0_PIN, RFID_2_D1_PIN, RFID_NO_OE_PIN, rfid_2_d0_isr, rfid_2_d1_isr, rfid_timeout_handler);
	}

	if (en_uhf) {
		printf("Init UHF RFID...\n");
		uhf_set_param(EPC_MEMBANK, 0x01, 7);
		uhf_init(UHF_PORT, UHF_BAUDRATE, OE_PIN);
		pthread_create(&uhf_thread_id, NULL, uhf_thread, NULL);
	}

	if (en_camera) {
		printf("Init Camera...\n");
		camera_init();
	}

	printf("System Ready!\n");
	fflush(stdout);

	while(1) pause();
	return 0;
}

#endif
