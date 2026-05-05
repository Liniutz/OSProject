CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET1 = city_manager_output
TARGET2 = monitor_reports_output

all: $(TARGET1) $(TARGET2)

$(TARGET1): src/city_manager.c
        $(CC) $(CFLAGS) -o $(TARGET1) src/city_manager.c

$(TARGET2): src/monitor_reports.c
        $(CC) $(CFLAGS) -o $(TARGET2) src/monitor_reports.c

clean:
        rm -f $(TARGET1) $(TARGET2)
.PHONY: all clean
