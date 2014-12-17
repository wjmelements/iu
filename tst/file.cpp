#include "file.h"
#include "net.h"

#include <assert.h>
#include <string.h>

// use "normal" test mocks
#include "stdmocks.h"

int main() {
    init_server();
    assert(next_msg_now() == NULL);
    heartbeat_msg hmsg;
    send_msg(&hmsg, me());
    struct msg* rhmsg = next_msg();
    assert(rhmsg->type == HEARTBEAT);
    free(rhmsg);

    char buffer[1000];
    char buffr2[1000];

    int urand_fd = Open("/dev/urandom", O_RDONLY);
    Read(urand_fd, buffer, sizeof(buffer));
    Close(urand_fd);

    int fake_file = Open("/tmp", O_RDWR | O_EXCL | O_TMPFILE);
    Write(fake_file, buffer, sizeof(buffer));
    Lseek(fake_file, 0, SEEK_SET);
    send_file(fake_file, me());
    Close(fake_file);

    int recvd = recv_file();
    assert(recvd != -1);
    Read(recvd, buffr2, sizeof(buffr2));
    Close(recvd);

    assert(memcmp(buffer, buffr2, 1000) == 0);

    shutdown_server();
    return 0;
}
