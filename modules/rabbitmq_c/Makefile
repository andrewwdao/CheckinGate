# https://stackoverflow.com/questions/2734719/how-to-compile-a-static-library-in-linux
# https://www.geeksforgeeks.org/static-vs-dynamic-libraries/
# https://www.rapidtables.com/code/linux/gcc/gcc-l.html


COMPILER=gcc

TARGET=main

OBJ_DIR=obj
HEADERS_DIR=headers
DEPS_DIR=src
LIB_DEPS_DIR=libs

DEPS_=pir rabbitmq rfid uhf
DEPS=$(DEPS_:%=$(OBJ_DIR)/%.o)

LIB_DEPS_=amqp_api amqp_connection amqp_mem amqp_socket amqp_table amqp_tcp_socket amqp_time amqp_framing
LIB_DEPS=$(LIB_DEPS_:%=$(OBJ_DIR)/%.o)

CFLAGS=-lwiringPi

$(TARGET): $(OBJ_DIR)/$(TARGET).o $(LIB_DEPS) $(DEPS)
	$(COMPILER) -I$(HEADERS_DIR) -o $@ $^ $(CFLAGS)

# Build library object files from library source files
$(OBJ_DIR)/%.o: $(LIB_DEPS_DIR)/%.c
	$(COMPILER) -I$(HEADERS_DIR) -c $^ -o$@

# Build source object files from source files
$(OBJ_DIR)/%.o: $(DEPS_DIR)/%.c
	$(COMPILER) $(CFLAGS) -I$(HEADERS_DIR) -c $^ -o$@


clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/*.a $(TARGET)





# $(TARGET): $(OBJ_DIR)/$(TARGET).o $(OBJ_DIR)/librabbitmq.a
# 	$(COMPILER) -I$(HEADERS_DIR) -o $@ $(OBJ_DIR)/$(TARGET).o -L$(OBJ_DIR) -lrabbitmq

# Build static library from object files
# $(OBJ_DIR)/librabbitmq.a: $(DEPS) $(LIB_DEPS) $(OBJ_DIR)/amqp_framing.o
# 	ar rcs $@ $^

# Build object file for main target
# $(OBJ_DIR)/%.o: %.c
# 	$(COMPILER) -I$(HEADERS_DIR) -c $^ -o$@