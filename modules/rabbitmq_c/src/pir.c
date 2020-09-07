#include <stdint.h>
#include <stdio.h>
#include <wiringPi.h>

#define PIR_CNT 4

void (*ext_isr_handler)(char*) = NULL;

void pir_1_isr() {
    if (ext_isr_handler) ext_isr_handler("1");
    else {
        printf("1\n");
        fflush(stdout);
    }
}

void pir_2_isr() {
    if (ext_isr_handler) ext_isr_handler("2");
    else {
        printf("2\n");
        fflush(stdout);
    }
}

void pir_3_isr() {
    if (ext_isr_handler) ext_isr_handler("3");
    else {
        printf("3\n");
        fflush(stdout);
    }
}

void pir_4_isr() {
    if (ext_isr_handler) ext_isr_handler("4");
    else {
        printf("4\n");
        fflush(stdout);
    }
}


void pir_set_ext_isr_handler(void (*ext_isr_handler_)()) {
    ext_isr_handler = ext_isr_handler_;
}

void pir_init(int pir_1_pin, int pir_2_pin,int pir_3_pin, int pir_4_pin) {
                  
    wiringPiSetup();
    pinMode(pir_1_pin, INPUT);
    pinMode(pir_2_pin, INPUT);
    pinMode(pir_3_pin, INPUT);
    pinMode(pir_4_pin, INPUT);

    wiringPiISR(pir_1_pin, INT_EDGE_FALLING, pir_1_isr);
    wiringPiISR(pir_2_pin, INT_EDGE_FALLING, pir_2_isr);
    wiringPiISR(pir_3_pin, INT_EDGE_FALLING, pir_3_isr);
    wiringPiISR(pir_4_pin, INT_EDGE_FALLING, pir_4_isr);
}