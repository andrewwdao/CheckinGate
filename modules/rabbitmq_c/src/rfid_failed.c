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
#define DEAFULT_RFID_ID 0
#define MAX_RFID_READER 2

/** 
 * @note Wiegand defines:
 *        + a Pulse Width time between 20 μs and 100 μs (microseconds)
 *        + a Pulse Interval time between 200 μs and 20000 μs
 * Each bit takes 4-6 usec, so all 26 bit sequence would take < 200usec */
#define WIEGAND_BIT_INTERVAL_TIMEOUT_USEC 20000 /* interval between bits, typically 1000us */
// ------ Private function prototypes -------------------------
void d0_ISR(void);
void d1_ISR(void);
void add_bit_w26(uint8_t,uint8_t);
void timeout_handler();
void reset_sequence(uint8_t);
void reset_timer(uint8_t);

// ------ not Private variables -----------------------------------
uint8_t rfid_cnt = 0;

struct itimerval it_val[MAX_RFID_READER];
struct sigaction sa[MAX_RFID_READER];

struct wiegand_data {
    uint8_t p0, p1;             //parity 0, parity 1
    uint8_t p0_check, p1_check; //parity check for first and last bit
    uint8_t facility_code;
    uint16_t card_code;
    uint32_t full_code;
    uint8_t code_valid;
    uint8_t bitcount;
} wds[MAX_RFID_READER];

// ------ PUBLIC variable definitions -------------------------

//--------------------------------------------------------------
// FUNCTION DEFINITIONS

void handle_ext_isr(uint8_t bit, uint8_t id) {
    reset_timer(id);     //timeout waiting for next bit
    add_bit_w26(bit, id);
}
//--------------------------------------------------------------
/**
 *  @brief DATA0 Interrupt Routine 
 */
void d0_ISR(void)
{
    reset_timer(DEAFULT_RFID_ID);     //timeout waiting for next bit
    add_bit_w26(0, DEAFULT_RFID_ID);
}//end d0_ISR

//--------------------------------------------------------------
/** 
 * @brief DATA1 Interrupt Routine 
 */
void d1_ISR(void)
{
    reset_timer(DEAFULT_RFID_ID);     //timeout waiting for next bit
    add_bit_w26(1, DEAFULT_RFID_ID);
}//end d1_ISR

//--------------------------------------------------------------
/**
 * @brief Parse Wiegand 26bit format. Called wherever a new bit is read
 * @param bit: 0 or 1
*/
void add_bit_w26(uint8_t bit, uint8_t id)
{
    //---------------------Parity calculation
    if (wds[id].bitcount > 0 && wds[id].bitcount <= 12) {wds[id].p0 += bit;}
    else if (wds[id].bitcount >= 13 && wds[id].bitcount <= 24) {wds[id].p1 += bit;}

    //---------------------Code calculation
    if (wds[id].bitcount == 0) {
        wds[id].p0_check = bit; //get current bit for checksum later
    }
    else if (wds[id].bitcount <= 8) {
        wds[id].facility_code <<= 1;
        wds[id].facility_code |= bit;
    }
    else if (wds[id].bitcount < 25) {
        wds[id].card_code <<= 1;
        wds[id].card_code |= bit;
    }
    else if (wds[id].bitcount == 25) {
        wds[id].p1_check = bit; //get current bit for checksum later
        wds[id].full_code = wds[id].facility_code;
        wds[id].full_code = wds[id].full_code << 16;
        wds[id].full_code += wds[id].card_code;
        wds[id].code_valid = 1;
        //---------------------check parity
        if ((wds[id].p0 % 2) != wds[id].p0_check) {
            wds[id].code_valid = 0;
            fprintf(stderr, "Incorrect even parity bit (leftmost)\n");
        }
        else if ((!(wds[id].p1 % 2)) != wds[id].p1_check) {
            wds[id].code_valid = 0;
            fprintf(stderr, "Incorrect odd parity bit (rightmost)\n");
        }
    }// end else if wds[id].bitcount == 25
    else if (wds[id].bitcount > 25) {
        wds[id].code_valid = 0;
        reset_sequence(id);
    }

    //---------------------Increase bit counting
    if (wds[id].bitcount < WIEGAND_MAXBITS) {
        wds[id].bitcount++;
    }
}//end add_bit_w26

//--------------------------------------------------------------
/**
 * @brief Timeout from last bit read. Sequence may be completed or not
 *        If completed, then output the desired information
*/
void timeout_handler()
{
    if (wds[DEAFULT_RFID_ID].code_valid) { //if the received code is valid
        printf("0x%06X\n", wds[DEAFULT_RFID_ID].full_code);
    } else { //if the received code is NOT valid
        printf("CHECKSUM_FAILED\n");
    }//end if else
    fflush(stdout);
    
    reset_sequence(DEAFULT_RFID_ID);

} //end timeout_handler

//--------------------------------------------------------------
/**
 * @brief Reset sequence of the wiegand data
 *        (reset bit counting variable)
*/
void reset_sequence(uint8_t id)
{
    wds[id].bitcount = 0;
    wds[id].p0 = 0;
    wds[id].p1 = 0;
    wds[id].p0_check = 0;
    wds[id].p1_check = 0;
    wds[id].code_valid = 0;
    wds[id].facility_code = 0;
    wds[id].card_code = 0;
}//end reset_sequence

//--------------------------------------------------------------
/**
 * @brief timeout handler (usec), should fire after bit sequence has been read
 * @param usec: microsecond to wait before trigger timeout
*/
void reset_timer(uint8_t id)
{
    /** @note Time until next expiration */
    it_val[id].it_value.tv_sec = 0;
    it_val[id].it_value.tv_usec = WIEGAND_BIT_INTERVAL_TIMEOUT_USEC;

    /** @note Interval for periodic timer */
    it_val[id].it_interval.tv_sec = 0;
    it_val[id].it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &it_val[id], NULL) < 0) {
        perror("setitimer");
        exit(1);
    }
}//end reset_timer

//--------------------------------------------------------------
/**
 * @brief Initialize the whole rfid system including GPIOs, interrupts and handlers
 * @return 0 if succeed, 1 if failed
 * @param d0pin: DATA0 pin from wiegand protocol
 * @param d1pin: DATA1 pin from wiegand protocol
 * @param oepin: Enable pin for logic converter TXS0108E
 *               put -1 if used MOSFET converter
*/
int rfid_init(int d0pin, int d1pin, int oepin,
              void (*d0_ext_isr_)(), void (*d1_ext_isr_)(),void(*ext_timeout_handler_)())
{
    if (rfid_cnt == MAX_RFID_READER) return 1;
    
    // it_val = (struct itimerval*)realloc(it_val, sizeof(struct itimerval)*(rfid_cnt+1));
    // sa = (struct sigaction*)realloc(sa, sizeof(struct sigaction)*(rfid_cnt+1));
    // wds = (struct wiegand_data*)realloc(wds, sizeof(struct wiegand_data)*(rfid_cnt+1));

    //-------------- Setup wiegand timeout handler -------------
    /** @brief initialize and empty a signal set */
    sigemptyset(&sa[rfid_cnt].sa_mask);

    // if (ext_timeout_handler_) ext_timeout_handler_();
    
    /** @brief assign handler for sigaction */
    sa[rfid_cnt].sa_handler = ext_timeout_handler_ ? ext_timeout_handler_ : timeout_handler;

    /** @brief assign catching signal for sigaction*/
    if (sigaction(SIGALRM, &sa[rfid_cnt], NULL) < 0) {
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

    wiringPiISR(d0pin, INT_EDGE_FALLING, d0_ext_isr_ ? d0_ext_isr_ : d0_ISR);
    wiringPiISR(d1pin, INT_EDGE_FALLING, d1_ext_isr_ ? d1_ext_isr_ : d1_ISR);

    reset_sequence(rfid_cnt);
    
    ++rfid_cnt;
    
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
                wds[id].bitcount, get_bit_timediff_ns() / 1000);
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
    printf("\n*** Code Valid: %d\n", wds[id].code_valid);
    printf("*** Facility code: %d(dec) 0x%X\n", wds[id].facility_code,
           wds[id].facility_code);
    printf("*** Card code: %d(dec) 0x%X\n", wds[id].card_code, wds[id].card_code);
    printf("*** Full code: %d\n", wds[id].full_code);
    printf("*** Parity 0:%d Parity 1:%d\n\n", wds[id].p0_check, wds[id].p1_check);
    fflush(stdout);
}//end rfid_showAll
*/
