#include <sys/types.h>

enum ctl_t {
    LEARN_PORT,
    STATUSREQ,
    STATUS,
};

typedef struct iuctl_msg {
    union {
        long mtype;
        ctl_t ctype;
    };
    int status;
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
