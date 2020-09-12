#include <amqp.h>

int rabbitmq_init(const char*,const char*,const char*,int);
void close_connection();
void send_message(char*,char*,char*);

