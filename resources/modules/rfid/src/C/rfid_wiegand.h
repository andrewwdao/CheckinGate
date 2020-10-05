/** ------------------------------------------------------------*-
 * Wiegand Reader - header file
 * Tested with Gwiot 7304D2 RFID Reader(26 bit Wiegand mode) and RASPBERRY PI 3B+
 * (c) Spiros Ioannou 2017
 * (c) Minh-An Dao 2019-2020
 * (c) Anh-Khoi Tran 2020
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
 * @ref https://bitbucket.org/sivann/wiegand_rpi/src/0731e6ea733cf80148a519f90135473666f9672c/?at=master
 *      https://man7.org/linux/man-pages/man2/getitimer.2.html
 *      https://linux.die.net/man/2/setitimer
 *      https://pubs.opengroup.org/onlinepubs/009695399/functions/sigemptyset.html
 *      https://man7.org/linux/man-pages/man2/sigaction.2.html
 * 
 -------------------------------------------------------------- */
#ifndef __RFID_WIEGAND_H
#define __RFID_WIEGAND_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
// ------ Public constants ------------------------------------

// ------ Public function prototypes --------------------------
int rfid_init(int,int,int);
void rfid_showUsage(void);
// ------ Public variable -------------------------------------

#endif //__RFID_WIEGAND_H