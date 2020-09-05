TARGET=test_send
HEADERS=libs
DEPS_DIR=librabbitmq
DEPS=amqp_api.o amqp_connection.o amqp_mem.o amqp_socket.o amqp_table.o amqp_tcp_socket.o amqp_time.o amqp_consumer.o

$(TARGET): $(TARGET).o librabbitmq.a
	gcc -I$(HEADERS) -o $@ $(TARGET).o -L. -lrabbitmq

$(TARGET).o: $(TARGET).c
	gcc -I$(HEADERS) -c $^

librabbitmq.a: $(DEPS) amqp_framing.o
	ar rcs $@ $^

amqp_framing.o: $(DEPS_DIR)/amqp_framing.c
	gcc -Ilibs -c $(DEPS_DIR)/amqp_framing.c

%.o: $(DEPS_DIR)/%.c $(DEPS_DIR)/%.h
	gcc -I$(HEADERS) -c $^

%.o: $(DEPS_DIR)/%.c
	gcc -I$(HEADERS) -c $^

clean:
	rm -rf *.o *.a $(TARGET)
