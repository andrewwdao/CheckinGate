#define RFID_NO_EXT_ISR NULL
#define RFID_NO_EXT_TIMEOUT_HANDLER NULL
#define RFID_NO_OE_PIN -1
#define RFID_D0_BIT 0
#define RFID_D1_BIT 1

#define RFID_CREATE_ISR_HANDLER(id,bit) (){ \
    handle_isr(id, bit); \
}

uint8_t rfid_init(uint8_t,uint8_t,uint8_t,int8_t);
void rfid_showUsage(void);

