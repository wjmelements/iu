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
pid_t self_pid;
pid_t server_pid;

void destroy_iuctl() {
    msgctl(msg_q, IPC_RMID, NULL);
    Unlink(QUEUE_PATH);
}

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
    int fd = open(QUEUE_PATH, O_WRONLY | O_CREAT | O_TRUNC, UMASK);
    if(fd < 0) {
        perror(QUEUE_PATH);
    }
    self_pid = getpid();
    assert(self_pid > 0);
    char self_pid_str[24];
    Snprintf(self_pid_str, sizeof(self_pid_str), "%lu", self_pid);
    write(fd, self_pid_str, strlen(self_pid_str));
    Close(fd);
    init_iuctl(true);
}
int join_iuctl() {
    int fd = open(QUEUE_PATH, O_RDONLY);
    if (fd == -1) {
        return errno;
    }
    char server_pid_str[32];
    read(fd, server_pid_str, sizeof(server_pid_str));
    Close(fd);
    server_pid = atoi(server_pid_str);
    assert(server_pid > 0);
    if(kill(server_pid, 0) != 0) {
        return errno;
    }
    self_pid = getpid();
    assert(self_pid > 0);
    init_iuctl(false);
    return 0;
}
void shutdown_iuctl() {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtext.ctype = SHUTDOWNREQ;
    send_iuctl(&msg, sizeof(msg));
}
void init_iuctl_msg(iuctl_msg_t* msg) {
    bzero(msg, sizeof(*msg));
    msg->mtext.pid = self_pid;
    msg->mtype = server_pid;
}
/* TODO */
void status_iuctl() {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtext.ctype = STATUSREQ;
    send_iuctl(&msg, sizeof(msg));
    while (recv_iuctl(self_pid, &msg, sizeof(msg)) != 0);
    printf("STATUS: GOOD\n");
}
void addr_iuctl(nid_t nid, addr_t* addr) {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtext.ctype = LEARN_ADDR;
    msg.mtext.nid = nid;
    send_iuctl(&msg, sizeof(msg));
    msg.mtype = self_pid;
    msg.addr = *addr;
    send_iuctl(&msg, sizeof(msg));
}
/* In response to NETREQ, a server sends two messages; one with length
 * and the other with a null-terminated string.
 */
void net_iuctl() {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtext.ctype = NETREQ;
    send_iuctl(&msg, sizeof(msg));
    // FIXME something more intelligent than != 0
    while (recv_iuctl(self_pid, &msg, sizeof(msg)) != 0);
    size_t len = msg.mtext.len;
    iuctl_msg_t* string = (iuctl_msg_t*) Malloc(len);
    while (recv_iuctl(self_pid, string, len) != 0);
    fputs(&string->stext, stdout);
    free(string);
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
