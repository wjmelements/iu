#include "iuctl.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STATUS_RETRY 100

#define UMASK 0644
/* Cannot be 0 */
#define QUEUE_ID 42
#define QUEUE_PATH "/tmp/q"

static int msg_q;
static void init_iuctl(bool creat) {
    if(creat) {
        int fd = open(QUEUE_PATH, O_CREAT | O_RDWR);
        close(fd);
    }
    /* Cannot use directory */
    key_t key = ftok(QUEUE_PATH, QUEUE_ID);
    if (key == -1) {
        perror("ftok");
        exit(errno);
    }
    int flags = UMASK;
    if (creat) {
        flags |= IPC_CREAT;
    }
    msg_q = msgget(key, flags);
}
void init_iuctl() {
    init_iuctl(true);
}
void join_iuctl() {
    init_iuctl(false);
}
void destroy_iuctl() {
    msgctl(msg_q, IPC_RMID, NULL);
}
/* TODO */
void status_iuctl() {
    iuctl_msg_t msg;
    msg.ctype = STATUSREQ;
    send_iuctl(&msg, sizeof(msg));
}
void send_iuctl(void* msg, size_t size) {
    int res = msgsnd(msg_q, msg, size - sizeof(long), 0);
    if(res < 0) {
        perror("send_iuctl");
    }
}
int recv_iuctl(ctl_t mtype, void* buf, size_t size) {
    int res = msgrcv(msg_q, &buf, size - sizeof(long), mtype, IPC_NOWAIT);
    if(res == -1) {
        return errno;
    }
    return 0;
}
