#include "capitalC.h"
#include <sys/types.h>

enum ctl_t {
    ANY = 0,
    IUCTL_REQ, // any request from iuctl to server will have this mtype to simplify message retrieval
    LEARN_PORT,
};

enum req_t {
    STATUS = 1,
};

typedef struct iuctl_msg {
    union {
        long mtype;
        ctl_t ctype;
    };
    req_t req;
} iuctl_msg_t;

void init_iuctl();
void join_iuctl();
void destroy_iuctl();

// called by only iuctl
void status_iuctl();

// called by only service
void handle_iuctls();

/* Size should be the size of the entire message
 * struct, including the leading long */
void send_iuctl(void* msg, size_t size);
int recv_iuctl(ctl_t mtype, void* buf, size_t size);
