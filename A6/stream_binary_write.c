#include <stdio.h>
#include <stdlib.h>

#define LEN 50
#define ELEMENTS 3

typedef struct {
    int id;
    float data;
    char name[LEN];
} sAnalogData_t;

// Datos de ejemplo
sAnalogData_t aData[ELEMENTS] = {
    { 0, 3.1416, "Temperature0" },
    { 1, 1.18, "Humidity0" },
    { 2, 23.5, "Pressure0" }
};

// Función para escribir en el archivo binario
void fwrite_test() { 
    FILE *fout = fopen("AnalogData.bin", "wb"); // abrir en modo binario
    if (fout == NULL) {
        perror("Error abriendo archivo para escribir");
        exit(EXIT_FAILURE);
    }

    fwrite((void *)aData, sizeof(sAnalogData_t), ELEMENTS, fout);
    fclose(fout);

    printf("Archivo 'AnalogData.bin' creado correctamente.\n");
}

// Función para leer en sentido inverso
void fread_reverse_test() {
    FILE *fin = fopen("AnalogData.bin", "rb");
    if (fin == NULL) {
        perror("Error abriendo archivo para leer");
        exit(EXIT_FAILURE);
    }

    sAnalogData_t temp;

    // Mover cursor al final del archivo
    fseek(fin, 0, SEEK_END);
    long file_size = ftell(fin);
    int struct_size = sizeof(sAnalogData_t);
    int total_structs = file_size / struct_size;

    printf("\nLeyendo estructuras en sentido inverso:\n");

    for (int i = total_structs - 1; i >= 0; i--) {
        fseek(fin, i * struct_size, SEEK_SET);
        fread(&temp, struct_size, 1, fin);
        printf("ID: %d  Data: %.4f  Name: %s\n", temp.id, temp.data, temp.name);
    }

    fclose(fin);
}

int main() {
    fwrite_test();        // Paso 1: Escribir archivo
    fread_reverse_test(); // Paso 2: Leer en orden inverso
    return 0;
}

