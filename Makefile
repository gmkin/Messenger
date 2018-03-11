CC := gcc
SRC_C := client
SRC_S := server
SRC_CHAT := chat
BUILD := build
BIN := bin
INC := -I include
CFLAGS := -Wall -Werror

.PHONY: clean all debug client

setup:
	mkdir -p bin build

debug: CFLAGS += -g -DDBG
debug: all

all: setup $(SRC_C) $(SRC_CHAT)


_CLIENTF := $(shell find $(SRC_C) -type f -name *.c)
_OBJ_CLIENT := $(patsubst $(SRC_C)/%,$(BUILD)/%,$(_CLIENTF:.c=.o))

$(SRC_C): $(_OBJ_CLIENT)
	$(CC) $^ -o $(BIN)/$@

$(BUILD)/%.o: $(SRC_C)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(SRC_S): 
	python $(SRC_S)/server.py 5000 10 "Thanks for checking out my Messenger Application"

_CHATF := $(shell find $(SRC_CHAT) -type f -name *.c)
_OBJ_CHAT := $(patsubst $(SRC_CHAT)/%,$(BUILD)/%,$(_CHATF:.c=.o))

$(SRC_CHAT): $(_OBJ_CHAT)
	$(CC) $^ -o $(BIN)/$@

$(BUILD)/%.o: $(SRC_CHAT)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	$(RM) -r $(BUILD) $(BIN)
