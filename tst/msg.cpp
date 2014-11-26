#include "msg.h"

#include <assert.h>

nid_t me() {
    return 42;
}

int main() {
    heartbeat_msg msg;
    assert(msg.type == HEARTBEAT);
    assert(msg.length == sizeof(msg));
    return 0;
}
