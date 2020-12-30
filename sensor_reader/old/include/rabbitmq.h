#include <amqp.h>

void rabbitmq_set_connection_params(const char*,const char*,const char*,int);
int rabbitmq_init();
void close_connection();
void send_message(char*,char*,char*);

