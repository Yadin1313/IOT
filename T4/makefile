CC = gcc
CFLAGS = -Wall -O2
TARGET = led_stream

all: $(TARGET)

$(TARGET): led_stream.c
	$(CC) $(CFLAGS) -o $(TARGET) led_stream.c

clean:
	rm -f $(TARGET)

