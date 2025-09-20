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

    // Crear memoria compartida
    int shm_id = shmget(SHM_KEY, 4096, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(-1);
    }

    buff = shmat(shm_id, 0, 0);
    if (buff == (char *)-1) {
        perror("shmat");
        exit(-1);
    }

    // Abrir o crear semáforo
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(-1);
    }

    printf("Writer listo. Escribe mensajes:\n");

    for (;;) {
        if (fgets(buff, MAX_LEN, stdin) == NULL)
            break;

        // Notificar al reader
        sem_post(sem);
    }

    shmdt(buff);
    sem_close(sem);

    return 0;
}

