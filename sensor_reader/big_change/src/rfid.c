#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/timeb.h>
#include <unistd.h>

#include <pthread.h>
#include <wiringPi.h>

#include <rabbitmq.h>
#include <sensor_reader.h>

#define max(a,b) (a>b ? a : b)

#define WIEGAND_VALID_BIT_CNT   26
#define WIEGAND_MAX_TIMEOUT     100000
#define WIEGAND_BIT_TIMEOUT     100
#define DEAULT_RFID_ID          0
#define USE_RFID                1
#define USE_UHF                 1

void reset_sequence(uint8_t);
void add_bit_w26(uint8_t, uint8_t);
uint8_t check_parity(uint8_t);

struct wiegand_data {
    uint8_t p0, p1;
    uint8_t p0_check, p1_check;
    unsigned long long last_bit_time;
    uint8_t bit_cnt;
    uint32_t tag_id;
} wds[4];
uint64_t now[4];

uint8_t waiting_next_bit[4];
pthread_t thread_id[4];

#if USE_RFID
    uint8_t rfid_1_id = 1;
    uint8_t rfid_2_id = 2;
#endif

#if USE_UHF
    uint8_t uhf_id = 3;
#endif

void* timeout_handler(void *id_ptr) {
    usleep(WIEGAND_MAX_TIMEOUT);

    uint8_t id = (uint8_t)(*(uint8_t*)id_ptr);

	if (check_parity(id) && wds[id].bit_cnt == WIEGAND_VALID_BIT_CNT) {
		printf("RFID %d: 0x%06X\n", id, wds[id].tag_id);
        fflush(stdout);

        char rfid_src[10];
        snprintf(rfid_src, 10, "rfid.%d", id);
        char routing_key[20];
        snprintf(routing_key, 20, "%s.%s", ROUTING_KEY_PREFIX, rfid_src);
        char data[20];
        snprintf(data, 20, "tag_id:0x%06X", wds[id].tag_id);

        #if en_rabbitmq
            char* formatted_message = format_message(get_current_time(), "rfid", rfid_src, data, OTHER_SENSOR_ID);
            send_message(formatted_message, EXCHANGE_NAME, routing_key);
            if (formatted_message != NULL) free(formatted_message);
        #endif
	} else {
        printf("RFID %d: CHECKSUM FAILED (%X, %d bits)\n", id, wds[id].tag_id, wds[id].bit_cnt);
    }

    waiting_next_bit[id] = 0;

    reset_sequence(id);
}

void handle_isr(uint8_t id, uint8_t bit) {
    if (!waiting_next_bit[id]) {
        waiting_next_bit[id] = 1;
        pthread_create(&thread_id[id], NULL, timeout_handler, (id == rfid_1_id ? &rfid_1_id : (id == rfid_2_id ? &rfid_2_id : &uhf_id))); // create a timeout handler thread for the wiegand data line
    }
    add_bit_w26(id, bit);
}

#if USE_RFID
    void rfid_1_d0_isr() {
        handle_isr(rfid_1_id, 0);
    }

    void rfid_1_d1_isr() {
        handle_isr(rfid_1_id, 1);
    }

    void rfid_2_d0_isr() {
        handle_isr(rfid_2_id, 0);
    }

    void rfid_2_d1_isr() {
        handle_isr(rfid_2_id, 1);
    }
#endif

#if USE_UHF
    void uhf_d0_isr() {
        handle_isr(uhf_id, 0);
    }

    void uhf_d1_isr() {
        handle_isr(uhf_id, 1);
    }
#endif

void reset_sequence(uint8_t id) {
    wds[id].p0 = wds[id].p1 = 0;
    wds[id].last_bit_time = 0;
    wds[id].bit_cnt = 0;
    wds[id].tag_id = 0;
}

uint8_t check_parity(uint8_t id) {
    if ( (wds[id].p0 % 2) - wds[id].p0_check ||
        !(wds[id].p1 % 2) - wds[id].p1_check)
        return 0;
    return 1;
}

void add_bit_w26(uint8_t id, uint8_t bit) {
    // printf("%d ", wds[id].bit_cnt);
    struct timeb ts;
	ftime(&ts);
    unsigned long long now = (unsigned long long int)ts.time*1000ll + ts.millitm;

    if (now - wds[id].last_bit_time > WIEGAND_BIT_TIMEOUT) {
        reset_sequence(id);
    }

    wds[id].last_bit_time = max(wds[id].last_bit_time, now);

    if      (wds[id].bit_cnt >  0  && wds[id].bit_cnt <= 12) wds[id].p0 += bit;
    else if (wds[id].bit_cnt >= 13 && wds[id].bit_cnt <= 24) wds[id].p1 += bit;

    if      (wds[id].bit_cnt == 0)  wds[id].p0_check = bit;
    else if (wds[id].bit_cnt < 25)  wds[id].tag_id = (wds[id].tag_id << 1) | bit;
    else if (wds[id].bit_cnt == 25) wds[id].p1_check = bit;

    ++wds[id].bit_cnt;
}

uint8_t rfid_init(uint8_t id, uint8_t d0_pin, uint8_t d1_pin, int8_t oe_pin)
{
    if (id <= 0 || id > 3) {
        printf("ID should be in range [1, 3] inclusive\n");
        return 1;
    }

    wiringPiSetup();
    pinMode(d0_pin, INPUT);
    pinMode(d1_pin, INPUT);

    if (oe_pin>=0) {
        pinMode(oe_pin, OUTPUT);
        digitalWrite(oe_pin, LOW);
        digitalWrite(oe_pin, HIGH);
    }

    wiringPiISR(d0_pin, INT_EDGE_RISING, id == 1 ? rfid_1_d0_isr : (id == 2 ? rfid_2_d0_isr : uhf_d0_isr));
    wiringPiISR(d1_pin, INT_EDGE_RISING, id == 1 ? rfid_1_d1_isr : (id == 2 ? rfid_2_d1_isr : uhf_d1_isr));

    waiting_next_bit[id] = 0;
    reset_sequence(id);
    
    return 0;
}
