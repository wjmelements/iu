#include "iuctl.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SERVER_EXEC "bin/server"

#define STATUS_GOOD 1

void start_iu(void) {
    char* argv[] = {"bin/server", "1", "1"};
    execv(SERVER_EXEC, argv);
}

void test_send_recv(void) {
    iuctl_msg_t msg;
    msg.ctype = STATUS;
    send_iuctl(&msg, sizeof(msg));
    assert(recv_iuctl(ctl_t::STATUS, &msg, sizeof(msg)) == 0);
}

void test_get_status(void) {
    iuctl_msg_t msg;
    msg.ctype = STATUSREQ;
    send_iuctl(&msg, sizeof(msg));
    assert(recv_iuctl(ctl_t::STATUS, &msg, sizeof(msg)) == 0);
    assert(msg.status == STATUS_GOOD);
}

int main() {
    init_iuctl();
    pid_t pid = fork();
    if(pid == 0)
        start_iu();
    // Let the server start up
    sleep(1);

    // TESTS
    test_send_recv();

    kill(pid, SIGKILL);
    destroy_iuctl();
    return 0;
}
