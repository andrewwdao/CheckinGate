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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <errno.h> //errno
#include <string.h> //strerror

#include <uhf.h>

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
#define MODE_CMD           0xA0
// Reader modes
#define STANDARD          0x00
#define WIEGAND34         0x02
#define WIEGAND26         0x03

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
 *  @brief Get checksum from uBuff
 *  @param uBuff the buffer to get checksum
 *  @param uLength length of the buffer
 *  @return checksum of the buffer
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
 *  @param arr the command to be formatted
 *  @param n length of the command array
 */
char* __format_command(char* arr, uint8_t n)
{
    n += 4;
    // [header, length, reader_address, command, checksum]
    char* command = malloc(n);

    command[0] = HEADER;
    command[1] = n - 2; // Message length count from third byte
    command[2] = DEFAULT_READER_ADDRESS;
    uint8_t i = 3;
    for (; i < n - 1; i++) command[i] = arr[i-3];
    command[i] = __get_checksum(command, i);
    //--- debug
    // for (int j = 0; j < n; j++) printf("%02x ", command[j]); printf("\n"); fflush(stdout);
    return command;
}

/**
 *  @brief Generate hex string from array
 *  @param arr array of data
 *  @param s start index
 *  @param e end index
 *  @return formatted string
 */
char* __get_hex_string(char* arr, uint8_t s, uint8_t e)
{
    char* str = malloc((e-s)*3 + 1);
    char* end_of_str = str;

    for (uint8_t i = s; i < e; i++)
        end_of_str += sprintf(end_of_str, "%02X", arr[i]);

    *end_of_str = '\0';
    return str;
}

/**
 *  @brief Read the whole packet
 *  @return Read packet
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
        
        *packet_len += 2; // return the full package length including header and package len itself

        return data;
    }
    *packet_len = 0;
    return "";
}

/**
 *  @brief reset the reader
 */
void __reset_reader() 
{
    // char reset_cmd[] = {RESET_CMD};
    // uint8_t len = (uint8_t)sizeof(reset_cmd)/sizeof(reset_cmd[0]);
    // serialPrintf(fd, __format_command(reset_cmd, len));
    char cmd[] = {HEADER, 0x03, DEFAULT_READER_ADDRESS, RESET_CMD, 0xec};
    serialPrintf(fd, cmd);
}

void __setmode_standard() 
{
    // char reset_cmd[] = {RESET_CMD};
    // uint8_t len = (uint8_t)sizeof(reset_cmd)/sizeof(reset_cmd[0]);
    // serialPrintf(fd, __format_command(reset_cmd, len));
    char cmd[] = {HEADER, 0x04, DEFAULT_READER_ADDRESS, MODE_CMD, STANDARD, 0xBD};
    serialPrintf(fd, cmd);
}

void __setmode_wiegand26() 
{
    // char reset_cmd[] = {RESET_CMD};
    // uint8_t len = (uint8_t)sizeof(reset_cmd)/sizeof(reset_cmd[0]);
    // serialPrintf(fd, __format_command(reset_cmd, len));
    char cmd[] = {HEADER, 0x04, DEFAULT_READER_ADDRESS, MODE_CMD, WIEGAND26, 0xB8};
    serialPrintf(fd, cmd);
}

void __setmode_wiegand34() 
{
    // char reset_cmd[] = {RESET_CMD};
    // uint8_t len = (uint8_t)sizeof(reset_cmd)/sizeof(reset_cmd[0]);
    // serialPrintf(fd, __format_command(reset_cmd, len));
    char cmd[] = {HEADER, 0x04, DEFAULT_READER_ADDRESS, MODE_CMD, WIEGAND34, 0x00};
    serialPrintf(fd, cmd);
}

/**
 * @brief Read tag
 */
char* uhf_read_tag()
{
    // printf("\nRead tag: ");
    char cmd[] = {READ_CMD, membank, word_address, word_cnt};
    uint8_t len = (uint8_t)sizeof(cmd)/sizeof(cmd[0]);
    char* formatted_cmd = __format_command(cmd, len);
    serialPrintf(fd, formatted_cmd);
    if (formatted_cmd != NULL) free(formatted_cmd);
    
    usleep(10000); // us

    uint8_t res_len;
    char* res = __read_response_packet(&res_len);
    
    // ---debug 
    // printf("\n");
    // print_hex_string(res, 0, res_len);

    if (res_len != 0)
    {
        if (res_len == 6) return "ERR";//printf("Error: 0x%02X\n", res[4]);
        else
        {
            if (__get_checksum(res, res_len-1) != res[res_len - 1]) printf("CHECKSUM FAILED");
            else {
                uint8_t data_len = res[6];
                uint8_t read_len = res[7 + data_len];

                // printf("Tag count: %d\n", res[4] + res[5]);
                // printf("PC: %s\n", __get_hex_string(res, 7, 9));
                // printf("EPC: %s\n", __get_hex_string(res, 9, 7 + data_len - read_len - 2));
                // printf("CRC: %s\n", __get_hex_string(res, 7 + data_len - read_len - 2, 7 + data_len - read_len));
                // printf("Full package: %s\n", __get_hex_string(res, 0, res_len));

                char* hex_str = __get_hex_string(res, 7 + data_len - read_len, 7 + data_len);
                printf("UHF Read data: 0x%s\n", hex_str);
                fflush(stdout);

                if (res != NULL) free(res);
                return hex_str;
            }
        }
    }
    fflush(stdout);
    return "ERR";
}

char* uhf_realtime_inventory()
{
    char cmd[] = {RT_INVENTORY_CMD, 255};
    uint8_t len = (uint8_t)sizeof(cmd)/sizeof(cmd[0]);
    char* formatted_cmd = __format_command(cmd, len);
    serialPrintf(fd, formatted_cmd);
    if (formatted_cmd != NULL) free(formatted_cmd);

    usleep(1000);

    uint8_t res_len;
    char* res = __read_response_packet(&res_len);

    if (res_len != 0)
    {
        // printf("res_len: %d data:", res_len);
        // for (int i = 0; i < res_len; i++) printf("%02X ", res[i]);
        // printf("\n");

        if (res_len == 6) return "ERR";//printf("Error: 0x%02X\n", res[4]);
        else if (res_len == 12) return "ERR"; // Filter out some random 12 bytes response, don't know why, fix later (maybe?)
        else
        {
            if (__get_checksum(res, res_len-1) != res[res_len - 1]) printf("CHECKSUM FAILED");
            else {
                // printf("\nPC: %s", __get_hex_string(res, 5, 7));
                // printf("\nRSSI: %s", __get_hex_string(res, res_len - 2, res_len - 1));
                // printf("\nEPC: %s", __get_hex_string(res, 7, res_len - 2));

                char* hex_str = __get_hex_string(res, 7, res_len - 2);
                printf("UHF EPC: 0x%s\n", hex_str);
                fflush(stdout);
                if (res != NULL) free(res);
                return hex_str;
            }
        }
    }
    fflush(stdout);
    return "ERR";
}

uint8_t uhf_set_param(uint8_t _membank, uint8_t _word_address, uint8_t _word_cnt)
{
    membank = _membank;
    word_address = _word_address;
    word_cnt = _word_cnt;

    if (((membank == TID_MEMBANK) && ((word_address + word_cnt) > TID_MEMBANK_WORD_LIM)) ||
        ((membank == EPC_MEMBANK) && ((word_address + word_cnt) > EPC_MEMBANK_WORD_LIM)) ||
        ((membank == USER_MEMBANK) && ((word_address + word_cnt) > USER_MEMBANK_WORD_LIM)))
        {
            printf("\nMembank word limit exceeded. (TID 12 words, EPC 8 words, USER 32 words)");
            return 1;
        }
    return 0;
}
//--------------------------------------------------------------
/**
 * @brief Initialize the whole uhf system including GPIOs, interrupts and handlers
 * @return 0 if succeed, 1 if failed
 * @param port: port defines for rs232 connection
 * @param baudrate: baudrate speed
 * @param oepin: Enable pin for logic converter TXS0108E
 *               put -1 if used MOSFET converter
*/
uint8_t uhf_init(const char* port, uint32_t baudrate, uint8_t oepin) 
{
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
        digitalWrite(oepin, HIGH); //set it low to high to make it works
    }

    serialFlush(fd);
    __reset_reader();

    return 0;
}//end uhf_init

//--------------------------------------------------------------
/**
 * @brief Show usage information for the user
*/
void uhf_show_usage()
{
    printf("\nHow to use:\n");
    printf("Type in the command line:\n");
    printf("\n\t./uhf [-h] [-p port] [-b baudrate] [-e OE-pin]\n\n");
    printf("With:\n");
    printf("\t-h : help\n");
    printf("\t-p port: port defines for rs232 connection\n");
    printf("\t-b baudrate: baudrate speed\n");
    printf("\t-e OE-pin: GPIO pin for output enable pin (wiringPi pin)\n\n");
    fflush(stdout);
}//end uhf_showUsage

//--------------------------------------------------------------
#endif //__UHF_RS232_C
