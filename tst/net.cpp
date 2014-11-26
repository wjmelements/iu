#include "net.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define ME 42
nid_t me() {
    return ME;
}

void believeDead(nid_t nid) {
    // unexpected
    assert(0);
}

static void basic_server_test() {
    port_t port = init_server(NULL);
    struct heartbeat_msg hmsg;

    assert(next_msg_now() == NULL);

    assert(send_msg(&hmsg, me()));
    struct msg* rmsg = next_msg();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    free(rmsg);

    assert(next_msg_now() == NULL);

    assert(send_msg(&hmsg, me()));
    rmsg = next_msg();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    free(rmsg);

    assert(next_msg_now() == NULL);

    shutdown_server();
}

int main() {
    for (size_t i = 0; i < 5; i++) {
        basic_server_test();
    }
    return 0;
}
