#include <amqp.h>

int rabbitmq_init(const char*,const char*,const char*,int,amqp_connection_state_t*,amqp_basic_properties_t*);
void close_connection(amqp_connection_state_t*);
void send_message(char*,char*,char*,amqp_connection_state_t,amqp_basic_properties_t*);

