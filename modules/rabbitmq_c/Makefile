TARGET=test_send
HEADERS=libs
DEPS_DIR=librabbitmq
DEPS=amqp_api.o amqp_connection.o amqp_mem.o amqp_socket.o amqp_table.o amqp_tcp_socket.o amqp_time.o amqp_consumer.o amqp_framing.o

$(TARGET): $(TARGET).o librabbitmq.a
	gcc -I$(HEADERS) -o $@ $(TARGET).o -Llibrabbitmq -lrabbitmq

$(TARGET).o: $(TARGET).c
	gcc -I$(HEADERS) -c $^

librabbitmq.a: $(DEPS)
	ar rcs $@ $^

%.o: $(DEPS_DIR)/%.c $(DEPS_DIR)/%.h
	gcc -I$(HEADERS) -c $^

%.o: $(DEPS_DIR)/%.c
	gcc -I$(HEADERS) -c $^


# amqp_socket.o: amqp_socket.c amqp_socket.h
# 	gcc -c -o $@ $^

# lib1.o: lib1.c lib1.h
# 	gcc -c -o $@ $<

# lib2.o: lib2.c lib2.h
# 	gcc -c -o $@ $<

clean:
	rm -rf *.o *.a $(TARGET)
