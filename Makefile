CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L
TARGET1 = city_manager_output
TARGET2 = monitor_reports_output
TARGET3 = city_hub_output
TARGET4 = scorer_output

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

$(TARGET1): src/city_manager.c
	$(CC) $(CFLAGS) -o $(TARGET1) src/city_manager.c

$(TARGET2): src/monitor_reports.c
	$(CC) $(CFLAGS) -o $(TARGET2) src/monitor_reports.c

$(TARGET3): src/city_hub.c
	$(CC) $(CFLAGS) -o $(TARGET3) src/city_hub.c

$(TARGET4): src/scorer.c
	$(CC) $(CFLAGS) -o $(TARGET4) src/scorer.c

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) city_hub monitor_reports .monitor_pid
.PHONY: all clean
