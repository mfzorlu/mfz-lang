# MFZ Lang — Makefile
# Kullanım: make | make clean | make run FILE=program.mfz

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
LDFLAGS = -lm
SRC_DIR = mfz
TARGET  = $(SRC_DIR)/mfz

SRCS = $(SRC_DIR)/lexer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/interpreter.c \
       $(SRC_DIR)/main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

run: $(TARGET)
	@if [ -z "$(FILE)" ]; then echo "Kullanım: make run FILE=program.mfz"; exit 1; fi
	./$(TARGET) $(FILE)

clean:
	rm -f $(TARGET) $(SRC_DIR)/*.o

.PHONY: all run clean
