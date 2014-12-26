#include "iuctl.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#define UMASK 0644
/* Cannot be 0 */
#define QUEUE_ID 42
#define QUEUE_PATH "./.iuctl_q"

static int msg_q;
static pid_t self_pid;
static pid_t server_pid;
static void init_iuctl(bool creat) {
    if(creat) {
        int fd = Open(QUEUE_PATH, O_CREAT | O_RDWR);
        Close(fd);
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
    int fd = open(QUEUE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    char self_pid_str[32];
    readlink("/proc/self", self_pid_str, sizeof(self_pid_str));
    self_pid = atoi(self_pid_str);
    assert(self_pid > 0);
    write(fd, self_pid_str, strlen(self_pid_str));
    init_iuctl(true);
}
int join_iuctl() {
    int fd = open(QUEUE_PATH, O_RDONLY);
    if(fd < 0) {
        perror("open");
        return -1;
    }
    char server_pid_str[32];
    read(fd, server_pid_str, sizeof(server_pid_str));
    server_pid = atoi(server_pid_str);
    assert(server_pid > 0);
    char self_pid_str[32];
    readlink("/proc/self", self_pid_str, sizeof(self_pid_str));
    self_pid = atoi(self_pid_str);
    assert(self_pid > 0);
    printf("%d\n", self_pid);
    init_iuctl(false);
    return 0;
}
void destroy_iuctl() {
    msgctl(msg_q, IPC_RMID, NULL);
}
void init_iuctl_msg(iuctl_msg_t* msg) {
    bzero(msg, sizeof(*msg));
    msg->mtext.pid = self_pid;
    msg->mtype = server_pid;
}
void handle_iuctls() {
    iuctl_msg_t msg;
    if(recv_iuctl(self_pid, &msg, sizeof(msg)) == 0) {
        pid_t iuctl_pid = msg.mtext.pid;
        init_iuctl_msg(&msg);
        msg.mtype = iuctl_pid;
        send_iuctl(&msg, sizeof(msg));
    }
    return;
}
/* TODO */
void status_iuctl() {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtext.ctype = STATUSREQ;
    send_iuctl(&msg, sizeof(msg));
    while(recv_iuctl(self_pid, &msg, sizeof(msg)) != 0);
    printf("STATUS: GOOD\n");
}
void send_iuctl(void* msg, size_t size) {
    assert(size - sizeof(long) > 0);
    int res = msgsnd(msg_q, msg, size - sizeof(long), 0);
    if(res < 0) {
        perror("send_iuctl");
    }
}
int recv_iuctl(int mtype, void* buf, size_t size) {
    int res = msgrcv(msg_q, buf, size - sizeof(long), mtype, IPC_NOWAIT);
    if(res == -1) {
        return errno;
    }
    return 0;
}
