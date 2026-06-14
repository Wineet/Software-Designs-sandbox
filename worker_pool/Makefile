###Compiler flags
CC	:= gcc
CFLAGS	:= -Wall -g
LDFLAGS	:= 
SHAREDLIBFLAGS	:= -fPIC -shared

#Directories
SRC_DIR	:= src
BUILD_DIR	:= build
INC_DIR		:= inc
TEST_DIR	:= test

SRC	:= $(SRC_DIR)/* #workerpool_main.c
INC	:= -I$(INC_DIR)
TARGET	:= $(BUILD_DIR)/libworkerpool.so
TEST	:= $(TEST_DIR)/workerpool_test
TEST_SRC	:= $(TEST_DIR)/workerpool_test.c

TEST_LINKING_LIBS	:= -l workerpool \
			   -lpthread

build:
	mkdir -p $(BUILD_DIR)
	echo "***** compiling workerpool.so *********"
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) $(INC) $(SHAREDLIBFLAGS) -o $(TARGET)
	echo "***** compiling workerpool_test ********"
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
