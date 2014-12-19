#include <assert.h>
#include <stdio.h>

#include "iuctl.h"

void test_send_recv() {
    iuctl_msg_t msg;
    msg.ctype = STATUS;
    send_iuctl(&msg, sizeof(msg));
    assert(recv_iuctl(ctl_t::STATUS, &msg, sizeof(msg)) == 0);
}

int main() {
    init_iuctl();
    test_send_recv();
    destroy_iuctl();
    return 0;
}
