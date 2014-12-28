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

const nid_t NID1 = 42;
const nid_t NID2 = 43;
nid_t ME = NID1;
nid_t me() {
    return ME;
}
bool isServer() {
    return false;
}
datacenter getLocation() {
    return US_WEST;
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
    struct heartbeat_msg hmsg;
    struct msg* rmsg;
    assert(me() == NID1);
    const addr_t parent_addr = init_server();
    assert(parent_addr.family == AF_INET6);
    assert(next_msg_now() == NULL);
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        ME = NID2;
        init_server();
        assert(next_msg_now() == NULL);
        setNodeAddr(NID1, &parent_addr);
        assert(next_msg_now() == NULL);
        bool success = send_msg(&hmsg, NID1);
        assert(success);

        net_suspend(NID1);
        for (size_t i = 0; i < 100; i++) {
            assert(next_msg_now() == NULL);
            sched_yield();
        }
        net_resume(NID1);

        rmsg = next_msg();
        assert(rmsg != NULL);
        assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
        assert(memcmp(rmsg, &hmsg, hmsg.length) == 0);
        assert(memcmp(rmsg, &hmsg, rmsg->length) == 0);
        assert(msg_source() == NID1);
        free(rmsg);

        assert(next_msg_now() == NULL);
        success = send_msg(&hmsg, NID1);
        assert(success);

        shutdown_server();
        exit(EXIT_SUCCESS);
    }

    rmsg = next_msg();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    assert(msg_source() == NID2);
    free(rmsg);

    assert(next_msg_now() == NULL);
    bool success = send_msg(&hmsg, NID2);
    assert(success);

    rmsg = next_msg_same();
    assert(rmsg != NULL);
    assert(memcmp(rmsg, &hmsg, sizeof(hmsg)) == 0);
    free(rmsg);

    shutdown_server();

    int status;
    assert(pid == Wait(&status));
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
}

void dead_server_test() {
    const addr_t parent_addr = init_server();
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        ME = NID2;
        init_server();
        setNodeAddr(NID1, &parent_addr);
        heartbeat_msg hmsg;
        bool success = send_msg(&hmsg, NID1);
        assert(success);
        sched_yield();
        success = send_msg(&hmsg, NID1);
        assert(success);
        shutdown_server();
        exit(0);
    }
    struct msg* hmsg = next_msg();
    assert(hmsg != NULL);
    assert(hmsg->type == HEARTBEAT);
    assert(msg_source() == NID2);
    free(hmsg);

    Kill(pid, SIGKILL);
    int status;
    assert(Wait(&status) == pid);

    assert(dead.size() == 0);

    hmsg = next_msg_now();
    if (hmsg == NULL) {
        // they were killed before they could send it
        assert(WIFSIGNALED(status));
        assert(WTERMSIG(status) == SIGKILL);
        // we should have believed them dead
        assert(dead.size() == 1);
        assert(dead.count(NID2) == 1);
    } else {
        // then we have received their message
        assert(hmsg->type == HEARTBEAT);
        free(hmsg);
        assert(msg_source() == NID2);
        // they either exited safely or were killed before they could
        assert(WIFEXITED(status) || WIFSIGNALED(status));
        if (WIFEXITED(status)) {
            assert(WEXITSTATUS(status) == 0);
        }
        if (WIFSIGNALED(status)) {
            assert(WTERMSIG(status) == SIGKILL);
        }
        // they have not yet died according to wnet
        assert(dead.size() == 0);
    }
    assert(next_msg_now() == NULL);
    shutdown_server();
    // should be dead to us now
    assert(dead.size() == 1);
    assert(dead.count(NID2) == 1);
    dead.clear();
}

void iterative_test() {
    init_server();
    
    const size_t iterations = 10000;

    assert(next_msg_now() == NULL);

    for (size_t i = 0; i < iterations; i++) {
        heartbeat_msg hmsg;
        bool success = send_msg(&hmsg, me());
        assert(success);
    }
    for (size_t i = 0; i < iterations; i++) {
        struct msg* hmsg = next_msg_now();
        assert(hmsg != NULL);
        assert(hmsg->type == HEARTBEAT);
        free(hmsg);
    }
    assert(next_msg_now() == NULL);

    shutdown_server();
}

void stream_test() {
    const addr_t parent_addr = init_server();
    assert(next_msg_now() == NULL);
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        ME = NID2;
        init_server();
        setNodeAddr(NID1, &parent_addr);
        assert(next_msg_now() == NULL);

        struct heartbeat_msg hmsg;
        send_msg(&hmsg, NID1);

        struct msg* next = next_msg();
        assert(next != NULL);
        assert(next->type = HEARTBEAT);
        free(next);

        assert(next_msg_now() == NULL);
        send_msg(&hmsg, NID1);

        shutdown_server();
        exit(0);
    }

    struct msg* next = next_msg();
    assert(next != NULL);
    assert(next->type = HEARTBEAT);
    free(next);

    stream<msg*>* in = send_stream(NID2);
    assert(in != NULL);
    assert(next_msg_now() == NULL);
    struct heartbeat_msg* hmsg = (heartbeat_msg*) Malloc(sizeof(heartbeat_msg));
    new (hmsg) heartbeat_msg;
    in->put(hmsg);
    
    in->close();
    next = next_msg();
    assert(next != NULL);
    assert(next->type = HEARTBEAT);
    free(next);
    assert(next_msg_now() == NULL);

    delete in;
    shutdown_server();

    int status;
    assert(pid == Wait(&status));
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
}
void two_stream_test() {
    const addr_t parent_addr = init_server();
    assert(next_msg_now() == NULL);
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        ME = NID2;
        init_server();

        assert(next_msg_now() == NULL);

        stream<msg*>* in1 = send_stream(NID1);
        stream<msg*>* in2 = send_stream(NID1);
        assert(in1 != NULL);
        assert(in2 != NULL);
        assert(next_msg_now() == NULL);

        struct heartbeat_msg* hmsg1 = (heartbeat_msg*) Malloc(sizeof(heartbeat_msg));
        struct heartbeat_msg* hmsg2 = (heartbeat_msg*) Malloc(sizeof(heartbeat_msg));
        new (hmsg1) heartbeat_msg;
        new (hmsg2) heartbeat_msg;
        in1->put(hmsg1);
        in2->put(hmsg2);

        struct msg* ack = next_msg();
        assert(ack != NULL);
        assert(msg_source() == NID1);
        assert(ack->type == HEARTBEAT);

        in1->close();
        net_idle();
        in2->close();
        net_idle();
        delete in1, in2;
        exit(0);
    }
    struct msg* next = next_msg();
    assert(next != NULL);
    assert(next->type == HEARTBEAT);
    assert(msg_source() == NID2);
    free(next);
    
    for (size_t i = 0; i < 100; i++) {
        assert(next_msg_now() == NULL);
        sched_yield();
    }
    heartbeat_msg hmsg;
    bool success = send_msg(&hmsg, NID2);
    assert(success);

    next = next_msg();
    assert(next != NULL);
    assert(next->type == HEARTBEAT);
    free(next);

    assert(next_msg_now() == NULL);

    shutdown_server();

    int status;
    assert(pid == Wait(&status));
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
}

int main() {
    recycle_test();
    basic_server_test();
    for (size_t i = 0; i < 10000; i++) {
        recycle_test();
    }
    basic_server_test();
    for (size_t i = 0; i < 5; i++) {
        basic_server_test();
    }
    for (size_t i = 0; i < 500; i++) {
        dead_server_test();
    }
    for (size_t i = 0; i < 5; i++) {
        iterative_test();
    }
    for (size_t i = 0; i < 5; i++) {
        stream_test();
    }
    for (size_t i = 0; i < 5; i++) {
        two_stream_test();
    }

    return 0;
}
