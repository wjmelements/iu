#include "net.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#include <set>
using std::set;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define ME 42
nid_t me() {
    return ME;
}

set<nid_t> dead;
void believeDead(nid_t nid) {
    dead.insert(nid);
}
static void recycle_test() {
    init_server();
    shutdown_server();
    assert(dead.size() == 0);
}

static void basic_server_test() {
    port_t port = init_server();
    struct heartbeat_msg hmsg;

    assert(next_msg_now() == NULL);

    assert(send_msg(&hmsg, me()));
    struct msg* rmsg = next_msg();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    assert(msg_source() == me());
    free(rmsg);

    assert(next_msg_now() == NULL);

    assert(send_msg(&hmsg, me()));
    rmsg = next_msg();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    assert(msg_source() == me());
    free(rmsg);

    assert(send_msg(&hmsg, me()));
    rmsg = next_msg_same();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    free(rmsg);

    assert(next_msg_now() == NULL);

    shutdown_server();
    assert(dead.size() == 0);
}

void dead_server_test() {
    init_server();
    /*
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        init_server();
        Kill(SIGKILL
    } else {
        
    }
    */
    shutdown_server();
}

int main() {
    pid_t pid = Fork();
    recycle_test();
    basic_server_test();
    if (!pid) {
        for (size_t i = 0; i < 100000; i++) {
            recycle_test();
        }
        basic_server_test();
        exit(EXIT_SUCCESS);
    }
    for (size_t i = 0; i < 5; i++) {
        basic_server_test();
    }
    for (size_t i = 0; i < 5; i++) {
        dead_server_test();
    }

    int status;
    assert(wait(&status) == pid);
    assert(WEXITSTATUS(status) == EXIT_SUCCESS);
    return 0;
}
