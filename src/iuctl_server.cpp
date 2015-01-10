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
void iuctl_server_net(pid_t iuctl_pid) {
    // TODO flags for net_addresses
    int flags;
    size_t len;
    char* net_status = net_addresses(&len, flags);
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtype = iuctl_pid;
    msg.mtext.len = len + sizeof(msg.mtype);
    send_iuctl(&msg, sizeof(msg));
    iuctl_msg_t* smsg = (iuctl_msg_t*) Malloc(msg.mtext.len);
    init_iuctl_msg(smsg);
    smsg->mtype = iuctl_pid;
    strncpy(&smsg->stext, net_status, len);
    send_iuctl(smsg, msg.mtext.len);
    free(net_status);
    free(smsg);
}
void iuctl_server_status(pid_t iuctl_pid) {
    iuctl_msg_t msg;
    init_iuctl_msg(&msg);
    msg.mtype = iuctl_pid;
    send_iuctl(&msg, sizeof(msg));
}
void iuctl_server_addr(pid_t iuctl_pid, nid_t nid) {
    iuctl_msg_t msg;
    while (recv_iuctl(iuctl_pid, &msg, sizeof(msg)) != 0);
    setNodeAddr(nid, &msg.addr);
}
void iuctl_server_shutdown(pid_t iuctl_pid) {
    destroy_iuctl();
    exit(0);
}
void handle_iuctls() {
    iuctl_msg_t msg;
    if(recv_iuctl(self_pid, &msg, sizeof(msg)) == 0) {
        pid_t iuctl_pid = msg.mtext.pid;
        switch(msg.mtext.ctype) {
            case LEARN_ADDR:
                iuctl_server_addr(msg.mtext.pid, msg.mtext.nid);
                break;
            case NETREQ:
                iuctl_server_net(iuctl_pid);
                break;
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
