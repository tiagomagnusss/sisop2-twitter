CC = g++
CFLAGS = -O2 -Wall -lpthread
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

DEBUG :=
DEBUGF := $(if $(DEBUG),-g -ggdb3)

.PHONY: all clean

all: client server

CLIENT_DEPS += $(OBJ_DIR)/client.o
CLIENT_DEPS += $(OBJ_DIR)/packet.o
client: $(CLIENT_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

SERVER_DEPS += $(OBJ_DIR)/server.o
SERVER_DEPS += $(OBJ_DIR)/packet.o
server: $(SERVER_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf $(BIN_DIR)/* $(OBJ_DIR)/*.o

redo: clean all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(DEBUGF) -c -o $@ $< $(CFLAGS)