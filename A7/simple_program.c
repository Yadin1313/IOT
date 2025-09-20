#include <stdio.h>
#include <unistd.h>

#define MAX_CHAR	80
#define PIPE_IN		1
#define PIPE_OUT	0

int main(int argc, char * argcv[]){

	int fds[2];
	char buff[MAX_CHAR];
	char pipeBuffOut[MAX_CHAR];

	/*crea pipe file descriptor*/
	int ret = pipe(fds);

	/*Lee line de consola*/
	char * str = fgets(buff, MAX_CHAR, stdin);

	/*Escribe linea en entrada del pipe*/
	ret = write(fds[PIPE_IN], buff, MAX_CHAR);
	/*Lee la salida del pipe*/
	ret = read(fds[PIPE_OUT], pipeBuffOut, MAX_CHAR);

	/*Imprime la salida de pipe*/
	printf("%s", pipeBuffOut);

	return 0;
}	
