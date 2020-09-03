/** ------------------------------------------------------------*-
 * Wiegand Reader - function file
 * Tested with Gwiot 7304D2 uhf Reader(26 bit Wiegand mode) and RASPBERRY PI 3B+
 * (c) Spiros Ioannou 2017
 * (c) Minh-An Dao 2019-2020
 * (c) Anh-Khoi Tran 2020
 * version 2.00 - 28/08/2020
 *--------------------------------------------------------------
 * uhf reader using Wiegand 26 protocol.
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
#ifndef __UHF_RS232_C
#define __UHF_RS232_C
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "uhf_rs232.h"

// ------ Private constants -----------------------------------
#define BAUDRATE         115200
#define PORT             "/dev/serial0" // /dev/ttyAMA0
#define ACCESS_PASSWORD  [0x00, 0x00, 0x00, 0x00]

// Reader commands
#define RESET_CMD          0x70
#define READ_CMD           0x81
#define WRITE_CMD          0x82
#define RT_INVENTORY_CMD   0x89 // Real time inventory
#define GET_READER_ID_CMD  0x68 // Get reader identifier
#define BUZZER_CMD         0x7A

// Buzzer modes
#define BUZZER_QUIET      0x00
#define BUZZER_ROUND      0x01
#define BUZZER_TAG        0x02

// Membanks
#define RESERVED_MEMBANK  0x00
#define EPC_MEMBANK       0x01
#define TID_MEMBANK       0x02
#define USER_MEMBANK      0x03

// Membank word limits, a word is 2 bytes (16 bit)
#define EPC_MEMBANK_WORD_LIM   8
#define TID_MEMBANK_WORD_LIM   12
#define USER_MEMBANK_WORD_LIM  32

// Needed to format package
#define HEADER                    0xA0
#define DEFAULT_READER_ADDRESS    0x01
#define PUBLIC_ADDRESS            0xFF

// Indexes
#define HEADER_INDEX       0
#define MESSAGE_LEN_INDEX  1

// Error codes
#define COMMAND_SUCCESS       0x10
#define COMMAND_FAIL          0x11
#define TAG_INV_ERROR         0x31 // Tag inventory error
#define TAG_READ_ERROR        0x32 // Error reading tag
#define TAG_WRITE_ERROR       0x33 // Error writing tag
#define TAG_LOCK_ERROR        0x34 // Error locking tag
#define TAG_KILL_ERROR        0x35 // Error killing tag
#define NO_TAG_ERROR          0x36
#define BUFFER_EMPTY          0x38
#define PARAM_INVALID         0x41 // Invalid parameter
#define WORD_CNT_TOO_LONG     0x42 // WordCnt too long
#define MEMBANK_OUT_OF_RANGE  0x43
#define LOCK_OUT_OF_RANGE     0x44 // Lock region out of range

// ------ Private function prototypes -------------------------

// ------ Private variables -----------------------------------
static int fd;

static int membank=TID_MEMBANK;
static int word_address=0x00;
static int word_cnt=0x01;
// ------ PUBLIC variable definitions -------------------------

//--------------------------------------------------------------
// FUNCTION DEFINITIONS
//--------------------------------------------------------------
/**
 *  @brief Read datasheet for checksum function 
 */
char __get_checksum(char* uBuff, uint8_t uLength)
{
    uint8_t i, cs = 0;
    for (i=0; i<uLength; i++) cs = cs + uBuff[i];
    cs = ~cs + 1;
    return cs;
}

/**
 *  @brief format command to byte array
 */
const char* __format_command(char* arr, uint8_t n)
{
    n += 4;
    // [header, length, reader_address, command, checksum]
    char* command = malloc(n);

    uint8_t i = 0;
    command[i++] = HEADER;
    command[i++] = n - 2; // Message length count from third byte
    command[i++] = DEFAULT_READER_ADDRESS;
    for (; i < n - 1; i++) command[i] = arr[i-3];
    command[i] = __get_checksum(command, i);

    // for (int j = 0; j < n; j++) printf("%02x ", command[j]); printf("\n"); fflush(stdout);
    return command;
}

/**
 * @brief Generate hex string from array
 * @param arr Data
 *
 */
char* __get_hex_string(char* arr, uint8_t s, uint8_t e)
{
    char* str = malloc((e-s)*3 + 1);
    char* end_of_str = str;

    for (uint8_t i = s; i < e; i++)
        end_of_str += sprintf(end_of_str, "%02X ", arr[i]);

    *end_of_str = '\0';
    return str;
}

/**
 * @brief Read the whole packet
 * @return 
 */
char* __read_response_packet(uint8_t* packet_len)
{
    if (serialDataAvail(fd)) {
        char header = serialGetchar(fd);
        if (header != HEADER)
            printf("\nWarning (__read_response_packet): %02X is not packet header", header);

        *packet_len = serialGetchar(fd);

        char* data = malloc(*packet_len + 2);

        data[0] = header;
        data[1] = *packet_len;
        for (uint8_t i = 0; i < *packet_len; i++)
            data[i+2] = serialGetchar(fd);
        
        *packet_len += 2;
        return data;
    }

    // *res_len = serialDataAvail(fd);
    // for (uint8_t i = 0; i < *res_len; i++) data[i] = serialGetchar(fd);

    // if (*res_len == 0) printf("\nNo response");
    // else if (data[0] != HEADER)
    //     printf("\nWarning (__read_response_packet): %02X is not packet header", data[0]);

    *packet_len = 0;
    return "";
}

/**
 *  @brief reset the reader
 */
void __reset_reader() 
{
    // A0 03 01 70 EC
    // printf("\nReset reader: ");
    // fflush(stdout);

    uint8_t len = 1;
    char reset_cmd[] = {RESET_CMD};

    serialPrintf(fd, __format_command(reset_cmd, len));
}

/**
 * @brief Read tag
 */
char* read_tag()
{
    // printf("\nRead tag: ");
    uint8_t len = 4;
    char read_cmd[] = {READ_CMD, membank, word_address, word_cnt};
    serialPrintf(fd, __format_command(read_cmd, len));

    usleep(10000);
    // sleep(1);

    uint8_t res_len;
    char* res = __read_response_packet(&res_len);

    // printf("\n");
    // print_hex_string(res, 0, res_len);

    if (res_len != 0)
    {
        if (res_len == 6) printf("Error: 0x%02X\n", res[4]);
        else
        {
            if (__get_checksum(res, res_len-1) != res[res_len - 1]) printf("CHECKSUM FAILED");
            else {
                uint8_t data_len = res[6];
                uint8_t read_len = res[7 + data_len];

                // printf("Tag count: %d\n", res[4] + res[5]);
                // printf("PC: %s\n", __get_hex_string(res, 7, 9));
                printf("EPC: %s\n", __get_hex_string(res, 7, 7 + data_len - read_len - 2));
                // printf("CRC: %s\n", __get_hex_string(res, 7 + data_len - read_len - 2, 7 + data_len - read_len));
                // printf("Read data: %s\n", __get_hex_string(res, 7 + data_len - read_len, 7 + data_len));
                // printf("\n");
                // printf("Read data: %s\n", __get_hex_string(res, 7 + data_len - read_len, 7 + data_len));
                // printf("Read data: %s\n", __get_hex_string(res, 0, res_len));
            }
        }
        fflush(stdout);
    }
    

    return "HAHAHA";
}

void realtime_inventory()
{
    char rt_inv_cmd[] = {RT_INVENTORY_CMD, 255};
    serialPrintf(fd, __format_command(rt_inv_cmd, 2));

    usleep(1000);

    uint8_t res_len;
    char* res = __read_response_packet(&res_len);

    if (res_len != 0)
    {
        if (res_len == 6) printf("Error: 0x%02X\n", res[4]);
        else
        {
            if (__get_checksum(res, res_len-1) != res[res_len - 1]) printf("CHECKSUM FAILED");
            else {
                printf("\nPC: %s", __get_hex_string(res, 5, 7));
                printf("\nEPC: %s", __get_hex_string(res, 7, res_len - 2));
                // printf("\nRSSI: %s", __get_hex_string(res, res_len - 2, res_len - 1));
            }
        }
        fflush(stdout);
    }
}

int set_param(int _membank, int _word_address, int _word_cnt)
{
    membank = _membank;
    word_address = _word_address;
    word_cnt = _word_cnt;

    if (((membank == TID_MEMBANK) && ((word_address + word_cnt) > TID_MEMBANK_WORD_LIM)) ||
        ((membank == EPC_MEMBANK) && ((word_address + word_cnt) > EPC_MEMBANK_WORD_LIM)) ||
        ((membank == USER_MEMBANK) && ((word_address + word_cnt) > USER_MEMBANK_WORD_LIM)))
        {
            printf("\nMembank word limit exceeded. (TID 12 words, EPC 8 words, USER 32 words)");
            return -1;
        }
    return 0;
}
// int __has_tag()
// {
//     char dat[] = {READ_CMD, membank, word_address, word_cnt};
// }
//--------------------------------------------------------------
/**
 * @brief Initialize the whole uhf system including GPIOs, interrupts and handlers
 * @return 0 if succeed, 1 if failed
 * @param d0pin: DATA0 pin from wiegand protocol
 * @param d1pin: DATA1 pin from wiegand protocol
 * @param oepin: Enable pin for logic converter TXS0108E
 *               put -1 if used MOSFET converter
*/
int uhf_init(const char* port, int baudrate, int oepin) 
{
    printf("\nUHF init\n");
    fflush(stdout);
    //-------------- Open connection -------------
    /** @brief initialize serial */
    if ((fd = serialOpen(port, baudrate)) < 0)
    {
        fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
        return 1 ;
    }
    /** @brief initialize wiringPi */
    if (wiringPiSetup() == -1)
    {
        fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
        return 1 ;
    }
    //---------------------- Setup Enable pin -----------------------
    if (oepin>0) {
        pinMode(oepin, OUTPUT);
        digitalWrite(oepin, LOW);
        // sleep(0.3);
        digitalWrite(oepin, HIGH);
    }

    __reset_reader();

    return 0;
}//end uhf_init

//--------------------------------------------------------------
/**
 * @brief Show usage information for the user
*/
void uhf_showUsage()
{
    printf("\nHow to use:\n");
    printf("Type in the command line:\n");
    printf("\n\t./uhf_main [-h] [-0 D0-pin] [-1 D1-pin] [-e OE-pin]\n\n");
    printf("With:\n");
    printf("\t-h : help\n");
    printf("\t-0 D0-pin: GPIO pin for data0 pulse (wiringPi pin)\n");
    printf("\t-1 D1-pin: GPIO pin for data1 pulse (wiringPi pin)\n");
    printf("\t-e OE-pin: GPIO pin for output enable pin (wiringPi pin)\n\n");
    fflush(stdout);
}//end uhf_showUsage

//--------------------------------------------------------------
#endif //__UHF_RS232_C
