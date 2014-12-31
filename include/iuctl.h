#ifndef IUCTL_H
#define IUCTL_H
#include "capitalC.h"
#include <sys/types.h>

enum ctl_t {
    LEARN_PORT,
    STATUSREQ,
    STATUS,
    SHUTDOWNREQ,
};

typedef struct iuctl_msg {
    long mtype;
    struct {
        pid_t pid;
        ctl_t ctype;
        int status;
    } mtext;
} iuctl_msg_t;

void init_iuctl();
int join_iuctl();
void destroy_iuctl();

void init_iuctl_msg(iuctl_msg_t* msg);
// called by only iuctl
void status_iuctl();
void shutdown_iuctl();

/* Size should be the size of the entire message
 * struct, including the leading long */
void send_iuctl(void* msg, size_t size);
int recv_iuctl(int mtype, void* buf, size_t size);

#endif
