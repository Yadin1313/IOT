#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LEN 80

static void parent_process(int fd){
	
	char buff[MAX_LEN];
	int status;
	pid_t cpid;

	for(;;){
		char * str = fgets(buff, MAX_LEN, stdin);
		if (str == NULL)
			break;
		int ret = write(fd, buff, MAX_LEN);	
	}

	close(fd);
	cpid = wait(&status);
}

static void child_process(int fd){
	
	char buff[MAX_LEN];
	int ret;
	for(;;){
		ret = read(fd, buff, MAX_LEN);
		printf("C: recieved from parent: %s\n", buff);
	}
	close(fd);
}

int main(int argc, char * argv[]){
	pid_t ret;
	int pfd[2];

	int ret_pipe = pipe(pfd);
	ret = fork();

	if (ret == 0){
		child_process(pfd[0]);
	}else if (ret>0){
		parent_process(pfd[1]);
	}else {
		printf("Error creating process\n");
	}

	return 0;
}

