void pir_init(int8_t,int8_t,int8_t);
void pir_set_ext_isr(void(*)(uint8_t));
void pir_set_ext_state_reader(void*(*)(void*));