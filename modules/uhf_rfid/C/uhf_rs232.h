/** ------------------------------------------------------------*-
 * Wiegand Reader - header file
 * Tested with Gwiot 7304D2 RFID Reader(26 bit Wiegand mode) and RASPBERRY PI 3B+
 * (c) Anh-Khoi Tran 2020
 * (c) Minh-An Dao 2020
 * version 2.00 - 28/08/2020
 *--------------------------------------------------------------
 * RFID reader using Wiegand 26 protocol.
 * @note Use both 125Khz and 315Mhz Cards
 *
 *  ------ Pinout ------
 *  1(RED)    - VCC     - DC 9-12V
 *  2(BROWN)  - FORMAT  - Keep floating to use wiegand26, connect to GND to use weigang34
 *  3(BLUE)   - OUT     - 5V in normal condition, GND when card was scanned
 *  4(YELLOW) - Buzzer  - pull up automatically (5V), pull to GND to make it beep.
 *  5(ORANGE) - LED     - pull up automatically (5V) - RED, pull to GND to make it GREEN.
 *  6(GREEN)  - DATA0   - Wiegand DATA0 pin.
 *  7(WHITE)  - DATA1   - Wiegand DATA1 pin.
 *  8(BLACK)  - GND
 *
 * This is interrupt driven, no polling occurs.
 * After each bit is read, a timeout is set.
 * If timeout is reached read code is evaluated for correctness.
 *
 * Wiegand Bits:
 * EFFFFFFFFNNNNNNNNNNNNNNNNO
 * E: even parity of F-bits and leftmost 4 N-bits
 * F: Facility code
 * N: Card Number
 * O: odd parity of rightmost 12 N-bits
 *
 * @ref https://mlab.vn/index.php?_route_=15129-bai-4-lap-trinh-raspberry-pi-su-dung-cong-truyen-thong-uart.html
 * 
 -------------------------------------------------------------- */
#ifndef __UHF_RS232_H
#define __UHF_RS232_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> //sleep, usleep, getopt
// ------ Public constants ------------------------------------
// ------ Public function prototypes --------------------------
uint8_t uhf_init(const char*,uint32_t,uint8_t);
void uhf_showUsage(void);
uint8_t set_param(uint8_t,uint8_t,uint8_t);
char* read_tag();
char* realtime_inventory();
// ------ Public variable -------------------------------------

#endif //__UHF_RS232_H