CC = g++
CFLAGS = -std=c++11 -O2 -Wall -lpthread -lncurses
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
CLIENT_DEPS += $(OBJ_DIR)/Profile.o
CLIENT_DEPS += $(OBJ_DIR)/Notification.o
CLIENT_DEPS += $(OBJ_DIR)/ClientUI.o
app_client: $(CLIENT_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

SERVER_DEPS += $(OBJ_DIR)/Server.o
SERVER_DEPS += $(OBJ_DIR)/Packet.o
SERVER_DEPS += $(OBJ_DIR)/Communication.o
SERVER_DEPS += $(OBJ_DIR)/Profile.o
SERVER_DEPS += $(OBJ_DIR)/Notification.o
app_server: $(SERVER_DEPS)
	$(CC) $(DEBUGF) -o $(BIN_DIR)/$@ $^ $(CFLAGS)

clean:
	rm -rf $(BIN_DIR)/* $(OBJ_DIR)/*.o

redo: clean all

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(DEBUGF) -c -o $@ $< $(CFLAGS)
