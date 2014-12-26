#include <assert.h>
#include <stdio.h>

#include "iuctl.h"

void test_send_recv() {
    iuctl_msg_t msg;
}

int main() {
    init_iuctl();
    test_send_recv();
    destroy_iuctl();
    return 0;
}
