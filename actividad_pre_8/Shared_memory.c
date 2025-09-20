#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdlib.h>

#include "common.h"

#define MAX_LEN 80

int main(int argc, char * argv[]){

	char * buff;
	int ret;

	int shm_id = shmget(SHM_KEY, 4096, IPC_CREAT | 0666);
	if (shm_id == -1)
		exit (-1);

	buff = shmat(shm_id,0,0);

	for (;;){
		char * str = fgets(buff, MAX_LEN, stdin);
		if(str == NULL ) 
			break;
	}

	ret =shmdt(buff);
	if (ret == 0)
		printf("Memory dettached\n");

	return 0;

}
