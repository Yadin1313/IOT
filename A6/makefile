CC ?= gcc

APP_NAME = stream_read_reverse
OBJFILES = stream_read_reverse.o

all: $(APP_NAME)

$(APP_NAME): $(OBJFILES)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $^ -o $@ -g

clean:
	rm -f *.o $(APP_NAME) AnalogData.bin

fresh:
	make clean
	make all

















