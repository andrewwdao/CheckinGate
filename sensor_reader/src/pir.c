#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <wiringPi.h>

#define PIR_STATE_DELAY 1000

void (*pir_ext_isr)(uint8_t) = NULL;
void* (*pir_ext_state_reader)(void*) = NULL;
int pir_state_pin = -1;


void pir_1_isr() {
    if (pir_ext_isr) pir_ext_isr(1);
    else {
        printf("1\n");
        fflush(stdout);
    }
}

void pir_2_isr() {
    if (pir_ext_isr) pir_ext_isr(2);
    else {
        printf("2\n");
        fflush(stdout);
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

void pir_set_ext_isr(void (*ext_isr)(uint8_t)) {
    pir_ext_isr = ext_isr;
}

void pir_set_ext_state_reader(void* (*state_reader)(void*)) {
    pir_ext_state_reader = state_reader;
}

void pir_init(int8_t pir_1_pin, int8_t pir_2_pin, int8_t pir_3_pin) {
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
        if (pir_ext_state_reader == NULL) pir_ext_state_reader = pir_in_state_reader;
        pthread_create(&pir_3_tid, NULL, pir_ext_state_reader, NULL);
    }
}