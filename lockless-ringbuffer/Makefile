###Compiler flags
CC	:= gcc
CFLAGS	:= -Wall
LDFLAGS	:=
SHAREDLIBFLAGS	:= -fPIC -shared

#Directories
SRC_DIR	:= src
BUILD_DIR	:= build
INC_DIR		:= inc
TEST_DIR	:= test

SRC	:= $(SRC_DIR)/ringbuffer.c
INC	:= -I$(INC_DIR)
TARGET	:= $(BUILD_DIR)/libringbuffer_lib.so
TEST	:= $(TEST_DIR)/ringbuffer_test
TEST_SRC	:= $(TEST_DIR)/ringbuffer_test.c

TEST_LINKING_LIBS	:= -l ringbuffer_lib \
			   -lpthread

build:
	mkdir -p $(BUILD_DIR)
	echo "***** compiling ringbuffer_lib.so *********"
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) $(INC) $(SHAREDLIBFLAGS) -o $(TARGET)
	echo "***** compiling ringbuffer_test ********"
	$(CC) $(CFLAGS) $(LDFLAGS) $(TEST_SRC) $(INC) -o $(TEST) $(TEST_LINKING_LIBS) -L $(BUILD_DIR)
all:	
	build
	run
	

clean:
	rm -rf $(TARGET)
	rm -rf $(TEST)

run:
	LD_LIBRARY_PATH=$(BUILD_DIR) \
	./$(TEST)

.PHONY:	all build clean
