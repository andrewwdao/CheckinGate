#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <wiringPi.h>

#include <rabbitmq.h>
#include <uhf.h>
#include <sensor_reader.h>

#define PIR_STATE_DELAY 1000

#define USE_STATE_PIR   1

int pir_state_pin = -1;

uint8_t pir_flags[PIR_CNT+1]; //2 pir sensor but we want the index to start at 1
uint8_t pir_debounce_flag[PIR_CNT+1]; //2 pir sensor but we want the index to start at 1
pthread_t pir_thread_id;
int pir_state_debounce = 0;

int pir1_id = 1, pir2_id = 2;


uint64_t now[PIR_CNT+1];

void* pir_send_thread(void* arg) {
    #if en_uhf_rs23
		uhf_realtime_inventory();
	#endif

	uint8_t id = (arg != NULL ? *(uint8_t*)arg : PIR_STATE_ID);
    
    now[id] = get_current_time();

    #if en_camera
        char cmd[100];
        snprintf(cmd, 100, "./sensor_reader/src/cam %s %d %llu %d", IMAGE_DIR, 1, now[id], IMAGE_LIMIT);
        system(cmd);

        char cmd2[100];
        snprintf(cmd2, 100, "./sensor_reader/src/cam %s %d %llu %d", IMAGE_DIR, 2, now[id], IMAGE_LIMIT);
        system(cmd2);
    #endif

	#if en_rabbitmq
		char pir_src[10];
		snprintf(pir_src, 10, "pir.%d", id);
		char routing_key[30];
		snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, pir_src);
		char data[20];
		snprintf(data, 10, "pir_id:%d", id);

		send_message(format_message(now[id], "pir", pir_src, data, id),
				 	 EXCHANGE_NAME, routing_key);
	#endif

	// Remove these lines if threads are used
	id != PIR_STATE_ID ?
		usleep(PIR_DEBOUNCE) :
		usleep(pir_state_debounce ? pir_state_debounce : PIR_STATE_DEBOUNCE);
	// if (id != PIR_STATE_ID) usleep(PIR_DEBOUNCE);
	pir_debounce_flag[id] = 1;
}

void pir_1_isr() {
    if (pir_debounce_flag[pir1_id]) {
		pir_debounce_flag[pir1_id] = 0;

		printf("PIR: %d\n", pir1_id);

		#if en_rabbitmq
			pthread_create(&pir_thread_id, NULL, pir_send_thread, &pir1_id);
		#endif
	}
}

void pir_2_isr() {
    if (pir_debounce_flag[pir2_id]) {
		pir_debounce_flag[pir2_id] = 0;

		printf("PIR: %d\n", pir2_id);

		#if en_rabbitmq
			pthread_create(&pir_thread_id, NULL, pir_send_thread, &pir2_id);
		#endif
	}
}


void* pir_3_reader(void* arg) {
	while (1) {
        if (pir_debounce_flag[PIR_STATE_ID] && !digitalRead(PIR_3_PIN)) {
			#if en_uhf_rs232
				uhf_realtime_inventory();
			#endif
			
			pir_debounce_flag[PIR_STATE_ID] = 0;
            printf("PIR: %d\n", PIR_STATE_ID);
            fflush(stdout);

			now[PIR_STATE_ID] = get_current_time();

			#if en_rabbitmq
				pir_send_thread(NULL);
			#endif
        }
        // usleep(pir_state_debounce ? pir_state_debounce : PIR_STATE_DEBOUNCE);
		usleep(100);
    }
}

void* pir_in_state_reader(void* arg) {
    while (1) {
        // printf("PIN: %d VAL: %d\n", pir_state_pin, digitalRead(pir_state_pin));
        if (!digitalRead(pir_state_pin)) {
            printf("3\n");
            fflush(stdout);
        }
        usleep(PIR_STATE_DELAY);
    }
}

void pir_init(int8_t pir_1_pin, int8_t pir_2_pin, int8_t pir_3_pin) {
    for (uint8_t i=0;i<=PIR_CNT;i++) *(pir_flags+i)= !(*(pir_debounce_flag+i) = 1);

    wiringPiSetup();
    if (pir_1_pin >= 0) {
        pinMode(pir_1_pin, INPUT);
        wiringPiISR(pir_1_pin, INT_EDGE_FALLING, pir_1_isr);
    }

    if (pir_2_pin >= 0) {
        pinMode(pir_2_pin, INPUT);
        wiringPiISR(pir_2_pin, INT_EDGE_FALLING, pir_2_isr);
    }

    if (pir_3_pin >= 0) {
        pinMode(pir_3_pin, INPUT);
        pir_state_pin = pir_3_pin;
        pthread_t pir_3_tid;
        pthread_create(&pir_3_tid, NULL, pir_3_reader, NULL);
    }
}