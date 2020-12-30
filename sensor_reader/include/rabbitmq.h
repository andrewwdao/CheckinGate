#include <amqp.h>

void rabbitmq_set_connection_params(const char*,const char*,const char*,int);
int rabbitmq_init();
void close_connection();
void send_message(char*,char*,char*);
char* format_message(uint64_t now, char* sensor, char* src, char* data, uint8_t sensor_id);
uint64_t get_current_time(void);
