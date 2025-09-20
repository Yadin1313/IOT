#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"

#define MAX_LEN 80

int main() {
    char *buff;

    // Conectarse a memoria compartida
    int shm_id = shmget(SHM_KEY, 0, 0);
    if (shm_id == -1) {
        perror("shmget");
        exit(-1);
    }

    buff = shmat(shm_id, 0, 0);
    if (buff == (char *)-1) {
        perror("shmat");
        exit(-1);
    }

    // Abrir semáforo existente
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(-1);
    }

    printf("Reader listo. Esperando mensajes...\n");

    for (;;) {
        sem_wait(sem);   // Bloquea hasta que writer avise
        printf("Reader leyó: %s", buff);
    }

    shmdt(buff);
    sem_close(sem);

    return 0;
}

