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
#include <wiringPi.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>


#define en_rabbitmq 0
#define en_pir		1
#define en_rfid	 	0
#define en_uhf		0
#define en_camera	0


// ------------------------- Constants -----------------------------------
// --- RabitMQ server infos
#define EXCHANGE_NAME 		"ex_sensors"
#define ROUTING_KEY_PREFIX	"event"
#define HOST				"demo1.gate.mekosoft.vn"
#define USERNAME			"admin"
#define PASSWORD			"admin"
#define PORT 				5672

// --- PIR parameters
#define PIR_DEBOUNCE 	   200000 //us
#define PIR_STATE_DEBOUNCE 100 //us
#define PIR_CNT	    	   3
#define PIR_1_PIN	       4  //wiringpi pin
#define PIR_2_PIN	       5  //wiringpi pin
#define PIR_3_PIN	       10  //wiringpi pin
#define PIR_STATE_ID	   3

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
#define IMAGE_LIMIT	  10000
#define IMAGE_DIR 	  "./web/public/images"
pthread_t camera_thread_id;
// ------------------------- Variables -----------------------------------
// --- PIR
uint8_t pir_flags[PIR_CNT+1]; //2 pir sensor but we want the index to start at 1
uint8_t pir_debounce_flag[PIR_CNT+1]; //2 pir sensor but we want the index to start at 1
pthread_t pir_thread_id;
int pir_state_debounce = 0;
// --- UHF
pthread_t uhf_thread_id;
// --- Keep track of time
uint64_t now;

// ------ Private function prototypes -------------------------
void* img_erase_thread(void*);
void* pir_send_thread(void*);
void* uhf_thread(void*);
void camera_init(void);
uint64_t get_current_time(void);
char* format_message(char*, char*, char*);
void pir_isr_handler(uint8_t);
void rfid_timeout_handler(uint8_t, uint32_t);
void uhf_read_handler(char*);
//--------------------------------------------------------------
// FUNCTION DEFINITIONS
//--------------------------------------------------------------
/**
 *  @brief separate thread for waiting and deleting first startup images for the camera
 *  @param arg void argument for the thread to be created
 *  @return void*
 */
void* img_erase_thread(void* arg)
{
	char cmd[50];
	usleep(2500000); //us - needed time for image to be created before removed
	snprintf(cmd, 50,  "rm -f %s/0_pir*.jpg", IMAGE_DIR);
	system(cmd);
}

/**
 *  @brief send pir interrupt signal to rabbitMQ
 *  @param arg id of the pir to be sent
 *  @return void*
 */
void* pir_send_thread(void* arg) {
	uint8_t id = (arg != NULL ? *(uint8_t*)arg : PIR_STATE_ID);
	pir_debounce_flag[id] = 0;

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
	if (id != PIR_STATE_ID) usleep(PIR_DEBOUNCE);
	pir_debounce_flag[id] = 1;
}

/**
 *  @brief UHF always-on thread to catch RS232 signal
 *  @note: This function will be replaced when wiegand26 protocol is applied for UHF reader
 *  @param arg void argument for the thread to be created
 *  @return void*
 */
void* uhf_thread(void* arg) {
	while(1) {
		char* data = uhf_realtime_inventory();
		
		if (!(data[0] == 'E' && data[1] == 'R' &&
			data[2] == 'R' && data[3] == '\0')) {
			uhf_read_handler(data);
		}
	}
	// uhf_read_tag();
	// while(1) {
	// 	char* data = uhf_read_tag();
	// 	uhf_read_handler(data);
	// }
}


/**
 *  @brief first connection to the camera stream to make it ready
 */
void camera_init(void)
{
	char cmd[100];
	snprintf(cmd, 100, "./sensor_reader/src/cam %s 1 0 %d", IMAGE_DIR, IMAGE_LIMIT);
	system(cmd);
	snprintf(cmd, 100, "./sensor_reader/src/cam %s 2 0 %d", IMAGE_DIR, IMAGE_LIMIT);
	pthread_create(&camera_thread_id, NULL, img_erase_thread, NULL);

}

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
 *  @brief ISR handler for PIR sensors
 *  @param id id of the pir to be sent
 */
void pir_isr_handler(uint8_t id) {
	printf("PIR: %d\n", id);

	if (pir_debounce_flag[id]) {
		fflush(stdout);
		now = get_current_time();
		// --- capture camera
		char cmd[100];
		snprintf(cmd, 100, "./sensor_reader/src/cam %s %d %llu %d", IMAGE_DIR, 1, now, IMAGE_LIMIT);
		system(cmd);

		char cmd2[100];
		snprintf(cmd2, 100, "./sensor_reader/src/cam %s %d %llu %d", IMAGE_DIR, 2, now, IMAGE_LIMIT);
		system(cmd2);

		pthread_create(&pir_thread_id, NULL, pir_send_thread, &id);
	}
}

void* pir_state_reader(void* arg) {
	while (1) {
        if (!digitalRead(PIR_3_PIN)) {
            printf("PIR: %d\n", PIR_STATE_ID);
            fflush(stdout);

			pthread_create(&pir_thread_id, NULL, pir_send_thread, NULL);
        }
        usleep(pir_state_debounce ? pir_state_debounce : PIR_STATE_DEBOUNCE);
    }
}

/**
 *  @brief timeout handler for wiegand26 protocol
 *  @param id id of the rfid being captured
 *  @param fullcode tag ID of the RFID
 */
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

	now = get_current_time();

	if (en_rabbitmq) {
		char* formatted_message = format_message("rfid", rfid_src, data);
		send_message(formatted_message, EXCHANGE_NAME, routing_key);
		if (formatted_message != NULL) free(formatted_message);
	}
}

/**
 *  @brief ISR handler for UHF RFID reader - RS232
 *  @param read_data data that the reader return
 */
void uhf_read_handler(char* read_data) {
	char* uhf_src = "rfid.3";
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, uhf_src);
	char data[150];
	snprintf(data, 150, "tag_id:0x%s", read_data);

	if (read_data != NULL) free(read_data);

	if (en_rabbitmq) {
		char* formatted_message = format_message("rfid", uhf_src, data);
		send_message(formatted_message, EXCHANGE_NAME, routing_key);
		if (formatted_message != NULL) free(formatted_message);
	}
}


int main(int argc, char** argv) {

	if (en_rabbitmq) {
		printf("Init RabbitMQ...\n");
		rabbitmq_set_connection_params(HOST, USERNAME, PASSWORD, PORT);
		rabbitmq_init();
	}

	if (en_pir) {
		if (argc > 0) pir_state_debounce = atoi(argv[1]);
		printf("Init PIRs...\n");
		for (uint8_t i=0;i<=PIR_CNT;i++) *(pir_flags+i)= !(*(pir_debounce_flag+i) = 1); //set all pir_flags to 0 and all pir_debounce_flag to 1 
		pir_set_ext_isr(pir_isr_handler);
		pir_set_ext_state_reader(pir_state_reader);
		pir_init(PIR_1_PIN, PIR_2_PIN, PIR_3_PIN);
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
