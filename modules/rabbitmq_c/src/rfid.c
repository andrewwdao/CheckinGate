#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <wiringPi.h>

#define max(a,b) (a>b ? a : b)

#define WIEGAND_VALID_BIT_CNT 26
#define WIEGAND_MAX_TIMEOUT 200000
#define DEAULT_RFID_ID 0

void d0_isr();
void d1_isr();
void reset_sequence(uint8_t);
void add_bit_w26(uint8_t, uint8_t);
uint8_t check_parity(uint8_t);


uint8_t rfid_cnt = 0;

struct wiegand_data {
    uint8_t p0, p1;
    uint8_t p0_check, p1_check;
    unsigned long long last_bit_time;
    uint8_t bit_cnt;
    uint32_t tag_id;
} wds[10];
uint8_t waiting_next_bit[10];
pthread_t thread_id[10];
void(*ext_timeout_handlers[10])(uint8_t,uint32_t);

void* timeout_handler(void *id_ptr) {
    usleep(100000);

    uint8_t id = *(uint8_t*)id_ptr;

    if (check_parity(id) && wds[id].bit_cnt == 26) {
        if (ext_timeout_handlers[id]) ext_timeout_handlers[id](id, wds[id].tag_id);
        else printf("0x%06X\n", wds[id].tag_id);
    } else {
        if (ext_timeout_handlers[id]) ext_timeout_handlers[id](id, 0);
        else printf("CHECKSUM FAILED\n");
    }
    
    reset_sequence(id);

    waiting_next_bit[id] = 0;
}

void handle_isr(uint8_t id, uint8_t bit) {
    if (!waiting_next_bit[id]) {
        pthread_create(&thread_id[id], NULL, timeout_handler, &id);
        waiting_next_bit[id] = 1;
    }

    add_bit_w26(id, bit);
}

// void handle_ext_isr(uint8_t id, uint8_t bit) {
//     add_bit_w26(id, bit);
//     handle_isr(id, bit);
// }

void d0_isr() {
    // add_bit_w26(DEAULT_RFID_ID, 0);
    handle_isr(DEAULT_RFID_ID, 0);
}

void d1_isr() {
    // add_bit_w26(DEAULT_RFID_ID, 1);
    handle_isr(DEAULT_RFID_ID, 1);
}

void reset_sequence(uint8_t id) {
    wds[id].p0 = wds[id].p1 = 0;
    wds[id].last_bit_time = 0;
    wds[id].bit_cnt = 0;
    wds[id].tag_id = 0;
}

uint8_t check_parity(uint8_t id) {
    if (wds[id].p0 % 2 - wds[id].p0_check ||
        !(wds[id].p1 % 2) - wds[id].p1_check ||
        wds[id].bit_cnt % WIEGAND_VALID_BIT_CNT)
        return 0;
    return 1;
}

void add_bit_w26(uint8_t id, uint8_t bit) {
    unsigned long long now = (unsigned long long)time(NULL);
    if (wds[id].last_bit_time > now + WIEGAND_MAX_TIMEOUT) {
        reset_sequence(id);
    }

    wds[id].last_bit_time = max(wds[id].last_bit_time, now);

    if (wds[id].bit_cnt > 0 && wds[id].bit_cnt <= 12) {wds[id].p0 += bit;}
    else if (wds[id].bit_cnt >= 13 && wds[id].bit_cnt <= 24) {wds[id].p1 += bit;}

    if (wds[id].bit_cnt == 0) {
        wds[id].p0_check = bit;
    } else if (wds[id].bit_cnt < 25) {
        wds[id].tag_id = (wds[id].tag_id << 1) | bit;
    } else if (wds[id].bit_cnt == 25) {
        wds[id].p1_check = bit;
        // if (check_parity(id)) {
        //     printf("0x%06X\n", wds[id].tag_id);
        // } else {
        //     printf("CHECKSUM FAILED\n");
        // }

        // reset_sequence(id);
    }

    ++wds[id].bit_cnt;
}

int rfid_init(int d0_pin, int d1_pin, int oe_pin,
              void(*d0_ext_isr_)(),void(*d1_ext_isr_)(),void(*ext_timeout_handler_)(uint8_t,uint32_t))
{
    // wds = (struct wiegand_data*)malloc(sizeof(struct wiegand_data)*(rfid_cnt+1));

    wiringPiSetup();
    pinMode(d0_pin, INPUT);
    pinMode(d1_pin, INPUT);

    if (oe_pin>0) {
        pinMode(oe_pin, OUTPUT);
        digitalWrite(oe_pin, LOW);
        sleep(0.3);
        digitalWrite(oe_pin, HIGH);
    }

    wiringPiISR(d0_pin, INT_EDGE_RISING, d0_ext_isr_ ? d0_ext_isr_ : d0_isr);
    wiringPiISR(d1_pin, INT_EDGE_RISING, d1_ext_isr_ ? d1_ext_isr_ : d1_isr);

    ext_timeout_handlers[rfid_cnt] = ext_timeout_handler_;

    waiting_next_bit[rfid_cnt] = 0;
    reset_sequence(rfid_cnt);
    
    ++rfid_cnt;
    
    return 0;
}