#define RFID_NO_EXT_ISR NULL
#define RFID_NO_EXT_TIMEOUT_HANDLER NULL
#define RFID_D0_BIT 0
#define RFID_D1_BIT 1

// extern uint8_t rfid_cnt;

extern uint8_t rfid_cnt;

#define RFID_CREATE_D0_ISR_HANDLER (){ \
    handle_isr(rfid_cnt, 0); \
}

#define RFID_CREATE_D1_ISR_HANDLER (){ \
    handle_isr(rfid_cnt, 1); \
}


int rfid_init(int,int,int,void(*)(),void(*)(),void(*)(uint8_t,uint32_t));
void rfid_showUsage(void);

void handle_isr(uint8_t, uint8_t);
void rfid_set_ext_timeout_handler(void(*)(),uint8_t);
