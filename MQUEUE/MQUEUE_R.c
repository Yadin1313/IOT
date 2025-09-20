#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdlib.h>
#include "common.h"

#define SIZE 100
#define MSGS 20

int main(int argc, char *argv[]) {
    char msg_buffer[SIZE];
    struct mq_attr attr;

    attr.mq_maxmsg = MSGS;
    attr.mq_msgsize = SIZE;
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;

    mqd_t msq = mq_open(MQUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (msq == (mqd_t)-1) {
        perror("mq_open");
        exit(-1);
    }

    for (;;) {
        int bytes_read = mq_receive(msq, msg_buffer, SIZE, 0);
        if (bytes_read == -1) {
            perror("mq_receive");
            break;
        }
        printf("%s", msg_buffer);
    }

    int ret = mq_close(msq);
    ret = mq_unlink(MQUEUE_NAME);

    return 0;
}

