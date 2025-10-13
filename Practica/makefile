# Compilador
CC = gcc
CFLAGS = -Wall -Wextra

# Archivos fuente
SRC = main.c client.c publish.c
OBJ = $(SRC:.c=.o)

# Ejecutable
TARGET = mqtt_client

# Regla por defecto
all: $(TARGET)

# Compilar el ejecutable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compilar archivos .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(OBJ) $(TARGET)

