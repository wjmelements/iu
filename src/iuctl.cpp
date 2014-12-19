#include "iuctl.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

static int msg_q;
static void init_iuctl(bool creat) {
    key_t key = ftok(".", 0);
    if (key == -1) {
        perror("ftok");
        exit(errno);
    }
    int flags = 0;
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

}
