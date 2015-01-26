#ifndef IUCTL_H
#define IUCTL_H
#include "capitalC.h"
#include "types.h"
#include <sys/types.h>

enum ctl_t {
    LEARN_ADDR,
    STATUSREQ,
    STATUS,
    NETREQ,
    SHUTDOWNREQ,
};

enum iu_status_t {
    IDLE,
    BUSY,
    ERROR
};

typedef struct iuctl_msg {
    long mtype;
    union {
        struct {
            pid_t pid;
            ctl_t ctype;
            union {
                iu_status_t status;
                size_t len;
                nid_t nid;
            };
        } mtext;
        char stext;
        addr_t addr;
    };
} iuctl_msg_t;

void init_iuctl();
int join_iuctl();
void destroy_iuctl();

void init_iuctl_msg(iuctl_msg_t* msg);
// called by only iuctl
void status_iuctl();
void shutdown_iuctl();
void net_iuctl();
void addr_iuctl(nid_t nid, addr_t* addr);

/* Size should be the size of the entire message
 * struct, including the leading long */
void send_iuctl(void* msg, size_t size);
int recv_iuctl(int mtype, void* buf, size_t size);

#endif
