/** ------------------------------------------------------------*-
 * Wiegand Reader - function file
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
#ifndef __RFID_WIEGAND_C
#define __RFID_WIEGAND_C
#include <sys/time.h>
#include <signal.h>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

// ------ Private constants -----------------------------------
#define WIEGAND_MAXBITS 27
/** 
 * @note Wiegand defines:
 *        + a Pulse Width time between 20 μs and 100 μs (microseconds)
 *        + a Pulse Interval time between 200 μs and 20000 μs
 * Each bit takes 4-6 usec, so all 26 bit sequence would take < 200usec */
#define WIEGAND_BIT_INTERVAL_TIMEOUT_USEC 20000 /* interval between bits, typically 1000us */
// ------ Private function prototypes -------------------------
void d0_ISR(void);
void d1_ISR(void);
void add_bit_w26(int8_t);
void timeout_handler(void);
void reset_sequence(void);
void reset_timer(uint32_t);

// ------ Private variables -----------------------------------
struct itimerval it_val;
struct sigaction sa;

struct wiegand_data {
    uint8_t p0, p1;             //parity 0, parity 1
    uint8_t p0_check, p1_check; //parity check for first and last bit
    uint8_t facility_code;
    uint16_t card_code;
    uint32_t full_code;
    uint8_t code_valid;
    uint8_t bitcount;
} wds;

void (*ext_timeout_handler)() = NULL;
// ------ PUBLIC variable definitions -------------------------

//--------------------------------------------------------------
// FUNCTION DEFINITIONS
//--------------------------------------------------------------
/**
 *  @brief DATA0 Interrupt Routine 
 */
void d0_ISR(void)
{
    reset_timer(WIEGAND_BIT_INTERVAL_TIMEOUT_USEC);     //timeout waiting for next bit
    add_bit_w26(0);
}//end d0_ISR

//--------------------------------------------------------------
/** 
 * @brief DATA1 Interrupt Routine 
 */
void d1_ISR(void)
{
    reset_timer(WIEGAND_BIT_INTERVAL_TIMEOUT_USEC);     //timeout waiting for next bit
    add_bit_w26(1);
}//end d1_ISR

//--------------------------------------------------------------
/**
 * @brief Parse Wiegand 26bit format. Called wherever a new bit is read
 * @param bit: 0 or 1
*/
void add_bit_w26(int8_t bit)
{
    //---------------------Parity calculation
    if (wds.bitcount > 0 && wds.bitcount <= 12) {wds.p0 += bit;}
    else if (wds.bitcount >= 13 && wds.bitcount <= 24) {wds.p1 += bit;}

    //---------------------Code calculation
    if (wds.bitcount == 0) {
        wds.p0_check = bit; //get current bit for checksum later
    }
    else if (wds.bitcount <= 8) {
        wds.facility_code <<= 1;
        wds.facility_code |= bit;
    }
    else if (wds.bitcount < 25) {
        wds.card_code <<= 1;
        wds.card_code |= bit;
    }
    else if (wds.bitcount == 25) {
        wds.p1_check = bit; //get current bit for checksum later
        wds.full_code = wds.facility_code;
        wds.full_code = wds.full_code << 16;
        wds.full_code += wds.card_code;
        wds.code_valid = 1;
        //---------------------check parity
        if ((wds.p0 % 2) != wds.p0_check) {
            wds.code_valid = 0;
            fprintf(stderr, "Incorrect even parity bit (leftmost)\n");
        }
        else if ((!(wds.p1 % 2)) != wds.p1_check) {
            wds.code_valid = 0;
            fprintf(stderr, "Incorrect odd parity bit (rightmost)\n");
        }
    }// end else if wds.bitcount == 25
    else if (wds.bitcount > 25) {
        wds.code_valid = 0;
        reset_sequence();
    }

    //---------------------Increase bit counting
    if (wds.bitcount < WIEGAND_MAXBITS) {
        wds.bitcount++;
    }
}//end add_bit_w26

//--------------------------------------------------------------
/**
 * @brief Timeout from last bit read. Sequence may be completed or not
 *        If completed, then output the desired information
*/
void timeout_handler(void)
{
    if (wds.code_valid) { //if the received code is valid
        printf("0x%X\n", wds.full_code);
        if (ext_timeout_handler) ext_timeout_handler(wds.full_code);
    } else { //if the received code is NOT valid
        printf("CHECKSUM_FAILED\n");
    }//end if else
    fflush(stdout);
    
    reset_sequence();

} //end timeout_handler

//--------------------------------------------------------------
/**
 * @brief Reset sequence of the wiegand data
 *        (reset bit counting variable)
*/
void reset_sequence(void)
{
    wds.bitcount = 0;
    wds.p0 = 0;
    wds.p1 = 0;
    wds.p0_check = 0;
    wds.p1_check = 0;
    wds.code_valid = 0;
    wds.facility_code = 0;
    wds.card_code = 0;
}//end reset_sequence

//--------------------------------------------------------------
/**
 * @brief timeout handler (usec), should fire after bit sequence has been read
 * @param usec: microsecond to wait before trigger timeout
*/
void reset_timer(uint32_t usec)
{ 
    /** @note Time until next expiration */
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = usec;

    /** @note Interval for periodic timer */
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &it_val, NULL) < 0) {
        perror("setitimer");
        exit(1);
    }
}//end reset_timer

void set_rfid_ext_timeout_handler(void (ext_timeout_handler_)()) {
    ext_timeout_handler = ext_timeout_handler_;
}

//--------------------------------------------------------------
/**
 * @brief Initialize the whole rfid system including GPIOs, interrupts and handlers
 * @return 0 if succeed, 1 if failed
 * @param d0pin: DATA0 pin from wiegand protocol
 * @param d1pin: DATA1 pin from wiegand protocol
 * @param oepin: Enable pin for logic converter TXS0108E
 *               put -1 if used MOSFET converter
*/
int rfid_init(int d0pin, int d1pin, int oepin) 
{
    //-------------- Setup wiegand timeout handler -------------
    /** @brief initialize and empty a signal set */
    sigemptyset(&sa.sa_mask);
    
    /** @brief assign handler for sigaction */
    sa.sa_handler = (void (*)(int))timeout_handler;

    /** @brief assign catching signal for sigaction*/
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    };
    //---------------------- Setup GPIOs -----------------------
    wiringPiSetup();
    pinMode(d0pin, INPUT);
    pinMode(d1pin, INPUT);

    if (oepin>0) {
        pinMode(oepin, OUTPUT);
        digitalWrite(oepin, LOW);
        sleep(0.3);
        digitalWrite(oepin, HIGH);
    }

    wiringPiISR(d0pin, INT_EDGE_FALLING, d0_ISR);
    wiringPiISR(d1pin, INT_EDGE_FALLING, d1_ISR);

    reset_sequence();
    return 0;
}//end rfid_init

//--------------------------------------------------------------
/**
 * @brief Show usage information for the user
*/
void rfid_showUsage()
{
    printf("\nHow to use:\n");
    printf("Type in the command line:\n");
    printf("\n\t./rfid_main [-h] [-0 D0-pin] [-1 D1-pin] [-e OE-pin]\n\n");
    printf("With:\n");
    printf("\t-h : help\n");
    printf("\t-0 D0-pin: GPIO pin for data0 pulse (wiringPi pin)\n");
    printf("\t-1 D1-pin: GPIO pin for data1 pulse (wiringPi pin)\n");
    printf("\t-e OE-pin: GPIO pin for output enable pin (wiringPi pin)\n\n");
    fflush(stdout);
}//end rfid_showUsage

//--------------------------------------------------------------
#endif //__RFID_WIEGAND_C



//static struct timespec wbit_tm; //for debug

/** @brief debug inside isr:
if (debug) {
        fprintf(stderr, "Bit:%02ld, Pulse 0, %ld us since last bit\n",
                wds.bitcount, get_bit_timediff_ns() / 1000);
        clock_gettime(CLOCK_MONOTONIC, &wbit_tm);
    }//end if
*/

/** @brief get the time differences from bit to bit (ns) - for debug only
unsigned uint32_t get_bit_timediff_ns() {
    struct timespec now, delta;
    unsigned uint32_t tdiff;

    clock_gettime(CLOCK_MONOTONIC, &now);
    delta.tv_sec = now.tv_sec - wbit_tm.tv_sec;
    delta.tv_nsec = now.tv_nsec - wbit_tm.tv_nsec;

    tdiff = delta.tv_sec * 1000000000 + delta.tv_nsec;

    return tdiff;
}//end get_bit_timediff_ns
*/
/** @brief show all information
void rfid_showAll() {
    printf("\n*** Code Valid: %d\n", wds.code_valid);
    printf("*** Facility code: %d(dec) 0x%X\n", wds.facility_code,
           wds.facility_code);
    printf("*** Card code: %d(dec) 0x%X\n", wds.card_code, wds.card_code);
    printf("*** Full code: %d\n", wds.full_code);
    printf("*** Parity 0:%d Parity 1:%d\n\n", wds.p0_check, wds.p1_check);
    fflush(stdout);
}//end rfid_showAll
*/
