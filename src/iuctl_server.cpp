#include "iuctl.h"
#include "iuctl_server.h"

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

extern pid_t self_pid;

void handle_iuctls() {
    iuctl_msg_t msg;
    if(recv_iuctl(self_pid, &msg, sizeof(msg)) == 0) {
        pid_t iuctl_pid = msg.mtext.pid;
        switch(msg.mtext.ctype) {
            case STATUSREQ:
                iuctl_server_status(iuctl_pid);
                break;
            case SHUTDOWNREQ:
                iuctl_server_shutdown(iuctl_pid);
            default:
                fprintf(stderr, "Received invalid message\n");
                break;
        }
    }
    return;
}
void iuctl_server_status(pid_t iuctl_pid) {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtype = iuctl_pid;
    send_iuctl(&msg, sizeof(msg));
}
void iuctl_server_shutdown(pid_t iuctl_pid) {
    destroy_iuctl();
    exit(0);
}
