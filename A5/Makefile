CC ?= gcc

APP_NAME = stream_string_test
OBJFILES = stream_string_test.o

all: $(APP_NAME)

$(APP_NAME): $(OBJFILES)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $^ -o $@ -g

clean:
	rm -f *.o $(APP_NAME)

fresh:
	make clean
	make all

