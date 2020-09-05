TARGET=test_send

OBJ_DIR=obj

DEPS_DIR=libs
DEPS_=amqp_api amqp_connection amqp_mem amqp_socket amqp_table amqp_tcp_socket amqp_time
DEPS=$(DEPS_:%=$(OBJ_DIR)/%.o)

$(TARGET): $(OBJ_DIR)/$(TARGET).o $(OBJ_DIR)/librabbitmq.a
	gcc -I$(DEPS_DIR) -o $@ $(OBJ_DIR)/$(TARGET).o -L$(OBJ_DIR) -lrabbitmq

$(OBJ_DIR)/librabbitmq.a: $(DEPS) $(OBJ_DIR)/amqp_framing.o
	ar rcs $@ $^

$(OBJ_DIR)/%.o: $(DEPS_DIR)/%.c
	gcc -I$(DEPS_DIR) -c $^ -o$@

$(OBJ_DIR)/%.o: %.c
	gcc -I$(DEPS_DIR) -c $^ -o$@

clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/*.a $(TARGET)
