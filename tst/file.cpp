#include "file.h"
#include "net.h"

#include <assert.h>
#include <string.h>

// use "normal" test mocks
#include "mocks/stdmocks.h"
void test_size(size_t size);

int main() {
    init_server();
    assert(next_msg_now() == NULL);
    heartbeat_msg hmsg;
    send_msg(&hmsg, me());
    struct msg* rhmsg = next_msg();
    assert(rhmsg->type == HEARTBEAT);
    free(rhmsg);

    test_size(1000);
    test_size(1024);
    test_size(0);
    test_size(1000);

    shutdown_server();
    return 0;
};

void test_size(size_t size) {
    char *buffer = (char*) Malloc(size);
    char *buffr2 = (char*) Malloc(size);

    int urand_fd = Open("/dev/urandom", O_RDONLY);
    Read(urand_fd, buffer, sizeof(buffer));
    Close(urand_fd);

    int fake_file = Open("/tmp", O_RDWR | O_EXCL | O_TMPFILE);
    Write(fake_file, buffer, size);
    Lseek(fake_file, 0, SEEK_SET);
    send_file(fake_file, me());
    Close(fake_file);

    int recvd = recv_file();
    assert(recvd != -1);
    Read(recvd, buffr2, size);
    Close(recvd);

    assert(memcmp(buffer, buffr2, size) == 0);

}
