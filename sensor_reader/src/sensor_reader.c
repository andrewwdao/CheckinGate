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
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>

#include <rabbitmq.h>
#include <pir.h>
#include <rfid.h>
#include <uhf.h>
#include <sensor_reader.h>
#include <CFHidApi.h>

pthread_t camera_thread_id;
// ------------------------- Variables -----------------------------------
// --- PIR

// --- UHF
pthread_t uhf_thread_id;
// --- Keep track of time
uint64_t now;

// ------ Private function prototypes -------------------------
void* img_erase_thread(void*);
void* uhf_thread(void*);
void camera_init(void);
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
 *  @brief UHF always-on thread to catch RS232 signal
 *  @note: This function will be replaced when wiegand26 protocol is applied for UHF reader
 *  @param arg void argument for the thread to be created
 *  @return void*
 */
void* uhf_thread(void* arg) {
	while(1) {
		char* data = uhf_read_rt_inventory();

		usleep(UHF_DELAY);

		if (data == NULL) continue;
		
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
	//pthread_create(&camera_thread_id, NULL, img_erase_thread, NULL);

}

/**
 *  @brief ISR handler for UHF RFID reader - RS232
 *  @param read_data data that the reader return
 */
void uhf_read_handler(char* read_data)
{
	char* uhf_src = "rfid.3";
	char routing_key[20];
	snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, uhf_src);
	char data[150];
	snprintf(data, 150, "tag_id:0x%s", read_data);

	// if (read_data != NULL) free(read_data);

	#if en_rabbitmq
		now = get_current_time();
		char* formatted_message = format_message(get_current_time(), "rfid", uhf_src, data, OTHER_SENSOR_ID);
		send_message(formatted_message, EXCHANGE_NAME, routing_key);
		// if (formatted_message != NULL) free(formatted_message);
	#endif
}

void setup_old_uhf()
{
	uhf_set_param(EPC_MEMBANK, 0x01, 7);
	uhf_init(UHF_PORT, UHF_BAUDRATE, OE_PIN);
	pthread_create(&uhf_thread_id, NULL, uhf_thread, NULL);
}

void cfuhf_callback(int msg, int tag_num, unsigned char *tag_data, int tag_data_len,unsigned char *devsn)
{
	if (msg == 0) printf("New device inserted\n");
	else if (msg == 1) printf("Device disconnected\n");
	else
	{
		int useless_data = 3;
		int epc_len = tag_data[0];
		char* tag = (char*)malloc(sizeof(char) * (epc_len*2 + 1));
		int i = 0;
		for (; i < tag_data[0]-useless_data; ++i)
			sprintf(tag+i*2, "%02X", tag_data[i+useless_data]);
		tag[i*2] = '\0';

		printf("UHF EPC: %s\n", tag);
		uhf_read_handler(tag);
	}
}

#if en_uhf_usb
void cfuhf_init()
{
	CFHid_OpenDevice();
	CFHid_SetCallback(cfuhf_callback);
	CFHid_StartRead(0xFF);
}

void setup_new_uhf()
{
	cfuhf_init();
}
#endif

void resetup()
{
	printf("RESETUP\n");
	fflush(stdout);

	#if en_pir
		printf("Init PIRs...\n");
		pir_init(PIR_1_PIN, PIR_2_PIN, PIR_NO_PIN);
	#endif

	#if en_rfid
		printf("Init RFIDs...\n");
		// --- RFID 1
		rfid_init(MAIN_RFID_1, RFID_1_D0_PIN, RFID_1_D1_PIN, RFID_NO_OE_PIN);
		// --- RFID 2
		rfid_init(MAIN_RFID_2, RFID_2_D0_PIN, RFID_2_D1_PIN, RFID_NO_OE_PIN);
	#endif

	// --- UHF
	#if en_uhf_w26
		printf("Init UHF RFID (Wiegand 26)...\n");
		rfid_init(MAIN_UHF, UHF_D0_PIN, UHF_D1_PIN, RFID_NO_OE_PIN);
		//rfid_init(MAIN_UHF, UHF_D0_PIN, UHF_D1_PIN, RFID_NO_OE_PIN, NULL, NULL, NULL);
	#endif
}

int main(int argc, char** argv) {

	#if en_rabbitmq
		printf("Init RabbitMQ...\n");
		rabbitmq_set_connection_params(HOST, USERNAME, PASSWORD, PORT);
		rabbitmq_init();
	#endif

	#if en_pir
		printf("Init PIRs...\n");
		pir_init(PIR_1_PIN, PIR_2_PIN, PIR_3_PIN);
	#endif

	#if en_rfid
		printf("Init RFIDs...\n");
		// --- RFID 1
		rfid_init(MAIN_RFID_1, RFID_1_D0_PIN, RFID_1_D1_PIN, RFID_NO_OE_PIN);
		// --- RFID 2
		rfid_init(MAIN_RFID_2, RFID_2_D0_PIN, RFID_2_D1_PIN, RFID_NO_OE_PIN);
	#endif

	// --- UHF
	#if en_uhf_w26
		printf("Init UHF RFID (Wiegand 26)...\n");
		rfid_init(MAIN_UHF, UHF_D0_PIN, UHF_D1_PIN, RFID_NO_OE_PIN);
		//rfid_init(MAIN_UHF, UHF_D0_PIN, UHF_D1_PIN, RFID_NO_OE_PIN, NULL, NULL, NULL);
	#elif en_uhf_rs232
		printf("Init UHF RFID (RS232)...\n");
		setup_old_uhf();
	#elif en_uhf_usb
		printf("Init UHF USB...\n");
		setup_new_uhf();
	#endif

	#if en_camera
		printf("Init Camera...\n");
		camera_init();
	#endif

	printf("System Ready!\n");
	fflush(stdout);

	while (1) pause();
	// while (1) {
    //     resetup();
    //     sleep(30);
    // }

	return 0;
}

#endif
