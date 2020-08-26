#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <memory.h>
// #include <stdint.h>
// #include <sys/time.h>
// #include <signal.h>
#include <wiringPi.h>

#define PIR_1_PIN 1
#define PIR_2_PIN 4
#define PIR_3_PIN 5
#define PIR_4_PIN 6

void send_rabbitmq(int code){
    char command[100];
    sprintf(command, "python3 send_to_rabbitmq.py pir %d pir_id:%d &", code, code);
    system(command);
}

void pir_1_isr() {
    printf("1\n");
    fflush(stdout);
    // send_rabbitmq(1);
}

void pir_2_isr() {
    printf("2\n");
    fflush(stdout);
    // send_rabbitmq(2);
}

void pir_3_isr() {
    printf("3\n");
    fflush(stdout);
    // send_rabbitmq(3);
}

void pir_4_isr() {
    printf("4\n");
    fflush(stdout);
    // send_rabbitmq(4);
}

void pir_init(int pir_1_pin, int pir_2_pin, int pir_3_pin, int pir_4_pin) {
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

int main() {
    pir_init(PIR_1_PIN, PIR_2_PIN, PIR_3_PIN, PIR_4_PIN);
    while(1) pause();
}
