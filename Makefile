CC=gcc
CFLAGS=-Wall -Wextra -g $$(curl-config --cflags)
LFLAGS=$$(curl-config --libs)

SRC_DIR=src
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_DIR=objs
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEPS_DIR=include
DEPS_FILES=$(wildcard $(DEPS_DIR)/*.h)
BIN=bin

TARGET=scraper

TARGET_FILES=$(patsubst %,$(BIN)/%,$(TARGET))

.PHONY: clean default

default : $(TARGET_FILES)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(DEPS_FILES)
	mkdir -p $(OBJ_DIR)/
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN)/% : $(OBJ_FILES)
	mkdir -p $(BIN)/
	$(CC) -o $@ $^ $(LFLAGS)
	
# $(TARGET) : $(OBJ_FILES)
# 	mkdir -p $(BIN)/
# 	$(CC) -o $(BIN)/$@ $^ $(LFLAGS)

# % : %.c $(LIBS)
# 	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)


clean : 
	-rm -rf $(OBJ_DIR)/
	-rm -rf $(BIN)/