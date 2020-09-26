#include <stdint.h>
#include <stdio.h>
#include <wiringPi.h>


void (*pir_ext_isr)(uint8_t) = NULL;

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

void pir_set_ext_isr(void (*ext_isr_)(uint8_t)) {
    pir_ext_isr = ext_isr_;
}

void pir_init(int8_t pir_1_pin, int8_t pir_2_pin) {
    wiringPiSetup();
    if (pir_1_pin >= 0) {
        pinMode(pir_1_pin, INPUT);
        wiringPiISR(pir_1_pin, INT_EDGE_FALLING, pir_1_isr);
    }

    if (pir_2_pin >= 0) {
        pinMode(pir_2_pin, INPUT);
        wiringPiISR(pir_2_pin, INT_EDGE_FALLING, pir_2_isr);
    }
}