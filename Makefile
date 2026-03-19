# MFZ Lang — Makefile
#
# make              → build binary
# make install      → build + install to system (Linux)
# make uninstall    → remove from system (Linux)
# make run FILE=x   → run a .mfz file
# make clean        → remove binary

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

install: $(TARGET)
	@chmod +x install.sh
	@./install.sh

uninstall:
	@chmod +x uninstall.sh
	@./uninstall.sh

run: $(TARGET)
	@if [ -z "$(FILE)" ]; then echo "Usage: make run FILE=program.mfz"; exit 1; fi
	./$(TARGET) $(FILE)

clean:
	rm -f $(TARGET) $(SRC_DIR)/*.o

.PHONY: all install uninstall run clean
