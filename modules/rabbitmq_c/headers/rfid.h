#define RFID_NO_EXT_ISR NULL
#define RFID_NO_EXT_TIMEOUT_HANDLER NULL
#define RFID_D0_BIT 0
#define RFID_D1_BIT 1

extern uint8_t rfid_cnt;
extern struct wiegand_data {
    uint8_t p0, p1;             //parity 0, parity 1
    uint8_t p0_check, p1_check; //parity check for first and last bit
    uint8_t facility_code;
    uint16_t card_code;
    uint32_t full_code;
    uint8_t code_valid;
    uint8_t bitcount;
} wds[];

#define RFID_CREATE_ISR_HANDLER(bit) (){ \
    handle_ext_isr(bit, rfid_cnt); \
}

#define RFID_CREATE_TIMEOUT_HANDLER(handler) () { \
    handler(rfid_cnt); \
}

int rfid_init(int,int,int,void(*)(),void(*)(),void(*)());
void rfid_showUsage(void);

void handle_ext_isr(uint8_t, uint8_t);
void rfid_set_ext_timeout_handler(void(*)(),uint8_t);
