void pir_init(int,int,int,int);
void pir_init_with_handlers(int,int,int,int,
                            void(*)(),void(*)(),void(*)(),void(*)());
void set_pir_isr_handler(uint8_t,void(*)());