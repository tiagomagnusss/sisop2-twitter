CC = g++
CFLAGS = -O2 -Wall -lpthread
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

DEBUG :=
DEBUGF := $(if $(DEBUG),-g -ggdb3)

.PHONY: all clean

all: app_client app_server

CLIENT_DEPS += $(OBJ_DIR)/Client.o
CLIENT_DEPS += $(OBJ_DIR)/Packet.o
CLIENT_DEPS += $(OBJ_DIR)/Communication.o
app_client: $(CLIENT_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

SERVER_DEPS += $(OBJ_DIR)/Server.o
SERVER_DEPS += $(OBJ_DIR)/Packet.o
SERVER_DEPS += $(OBJ_DIR)/Communication.o
app_server: $(SERVER_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf $(BIN_DIR)/* $(OBJ_DIR)/*.o

redo: clean all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(DEBUGF) -c -o $@ $< $(CFLAGS)