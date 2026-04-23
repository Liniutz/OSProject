CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = city_manager

all: $(TARGET)

$(TARGET): src/city_manager.c
	$(CC) $(CFLAGS) -o $(TARGET) src/city_manager.c

clean:
	rm -f $(TARGET)
	rm -rf active_reports-*

.PHONY: all clean
