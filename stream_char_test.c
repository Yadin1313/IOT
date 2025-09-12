#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN 80

void stream_string_test() {
    char buffer[LEN];
    FILE *fout, *fin;

    fout = fopen("test_file_string.txt", "w");
    if (fout == NULL) {
        perror("Error abriendo archivo");
        exit(EXIT_FAILURE);
    }

    fin = stdin;

    while (fgets(buffer, LEN, fin) != NULL) {
        fputs(buffer, fout);
    }

    fclose(fout);
    printf("Archivo 'test_file_string.txt' creado con éxito.\n");
}

int main() {
    stream_string_test();
    return 0;
}

