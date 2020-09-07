#include <stdint.h>
#include <stdio.h>
#include <wiringPi.h>

#define PIR_CNT 4

void default_pir_1_isr();
void default_pir_2_isr();
void default_pir_3_isr();
void default_pir_4_isr();

void (*isr_handlers[4])() = {
    &default_pir_1_isr,
    &default_pir_2_isr,
    &default_pir_3_isr,
    &default_pir_4_isr,
};

void default_pir_1_isr() {
    printf("1\n");
    fflush(stdout);
}

void default_pir_2_isr() {
    printf("2\n");
    fflush(stdout);
}

void default_pir_3_isr() {
    printf("3\n");
    fflush(stdout);
}

void default_pir_4_isr() {
    printf("4\n");
    fflush(stdout);
}


void set_pir_isr_handler(uint8_t pir_id, void (*isr_handler)()) {
    if (pir_id < 1 || pir_id > PIR_CNT) {
        printf("PIR ID must in range 1 - %d", PIR_CNT);
        return;
    }

    isr_handlers[--pir_id] = isr_handler;
}

void pir_init(int pir_1_pin, int pir_2_pin,int pir_3_pin, int pir_4_pin) {
                  
    wiringPiSetup();
    pinMode(pir_1_pin, INPUT);
    pinMode(pir_2_pin, INPUT);
    pinMode(pir_3_pin, INPUT);
    pinMode(pir_4_pin, INPUT);

    wiringPiISR(pir_1_pin, INT_EDGE_FALLING, default_pir_1_isr);
    wiringPiISR(pir_2_pin, INT_EDGE_FALLING, default_pir_2_isr);
    wiringPiISR(pir_3_pin, INT_EDGE_FALLING, default_pir_3_isr);
    wiringPiISR(pir_4_pin, INT_EDGE_FALLING, default_pir_4_isr);
}

void pir_init_with_handlers(int pir_1_pin, int pir_2_pin,
                            int pir_3_pin, int pir_4_pin,
                            void (*pir_1_isr_handler)(),
                            void (*pir_2_isr_handler)(),
                            void (*pir_3_isr_handler)(),
                            void (*pir_4_isr_handler)()) {
                  
    wiringPiSetup();
    pinMode(pir_1_pin, INPUT);
    pinMode(pir_2_pin, INPUT);
    pinMode(pir_3_pin, INPUT);
    pinMode(pir_4_pin, INPUT);

    wiringPiISR(pir_1_pin, INT_EDGE_FALLING, pir_1_isr_handler ? pir_1_isr_handler : default_pir_1_isr);
    wiringPiISR(pir_2_pin, INT_EDGE_FALLING, pir_2_isr_handler ? pir_2_isr_handler : default_pir_2_isr);
    wiringPiISR(pir_3_pin, INT_EDGE_FALLING, pir_3_isr_handler ? pir_3_isr_handler : default_pir_3_isr);
    wiringPiISR(pir_4_pin, INT_EDGE_FALLING, pir_4_isr_handler ? pir_4_isr_handler : default_pir_4_isr);
}