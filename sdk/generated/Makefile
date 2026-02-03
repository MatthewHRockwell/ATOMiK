# Makefile for ATOMiK C SDK

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I.

SRC = atomik/video/streaming/h264_delta.c
TEST = tests/test_h264_delta.c
TARGET = test_h264_delta

all: $(TARGET)

$(TARGET): $(SRC) $(TEST)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(TEST)

clean:
	rm -f $(TARGET)

.PHONY: all clean
