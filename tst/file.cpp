#include "file.h"
#include "net.h"

#include <assert.h>
#include <string.h>

// use "normal" test mocks
#include "mocks/nodemocks.h"
datacenter getLocation() {
    return US_WEST;
}
#define NID1 42
#define NID2 43
static nid_t ME = NID1;
nid_t me() {
    return ME;
}
void test_size(size_t size);

static size_t expecting = 0;
void believeDead(nid_t nid) {
    assert(expecting-- && believeDead);
}
static addr_t parent_addr;
int main() {
    parent_addr = init_server();
    assert(next_msg_now() == NULL);
    
    test_size(1000);
    test_size(1024);
    test_size(0);
    test_size(1000);
    test_size(2000);
    test_size(3000);
    test_size(10000);
    test_size(100000);

    shutdown_server();
    return 0;
};

void callback(void* carg) {
    struct ffarg_t* farg = (ffarg_t*) carg;
    volatile int* fd_put = (volatile int*) farg->arg;
    *fd_put = farg->fd;
    free(farg);
}
void test_size(size_t size) {
    char *buffer = (char*) Malloc(size);
    char *buffr2 = (char*) Malloc(size);

    int urand_fd = Open("/dev/urandom", O_RDONLY);
    Read(urand_fd, buffer, sizeof(buffer));
    Close(urand_fd);

    int fake_file = Open("/tmp", O_RDWR | O_EXCL | O_TMPFILE);
    Write(fake_file, buffer, size);
    Lseek(fake_file, 0, SEEK_SET);

    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();
        ME = NID2;
        init_server();
        setNodeAddr(NID1, &parent_addr);

        heartbeat_msg hmsg;
        bool success = send_msg(&hmsg, NID1);
        assert(success);

        send_file(fake_file, NID1);
        Close(fake_file);

        struct msg* ack = next_msg();
        assert(ack != NULL);
        assert(ack->type == HEARTBEAT);
        assert(msg_source() == NID1);
        free(ack);

        shutdown_server();
        exit(0);
    }
    Close(fake_file);

    expecting++;

    struct msg* rhmsg = next_msg();
    assert(rhmsg->type == HEARTBEAT);
    assert(msg_source() == NID2);
    free(rhmsg);

    volatile int recvd = 0;
    recv_file(callback, (void*) &recvd);
    while (recvd == 0) {
        net_idle();
        sched_yield();
        do_todos();
    }
    assert(recvd != -1);
    Read(recvd, buffr2, size);
    Close(recvd);

    struct heartbeat_msg ack;
    bool success = send_msg(&ack, NID2);
    assert(success);

    assert(memcmp(buffer, buffr2, size) == 0);
    Free(buffer);
    Free(buffr2);

    int status;
    assert(pid == Wait(&status));
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);

    assert(next_msg_now() == NULL);
    net_idle();
}
