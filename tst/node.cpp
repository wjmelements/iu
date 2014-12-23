#include "node.h"

#include "mocks/nodemocks.h"

#define NID 42
#define NID2 43
#define LOC US_WEST

extern const addr_t& getNodeAddr(nid_t nid);

size_t expectingDeaths = 0;
void believeDead(nid_t nid) {
    assert(expectingDeaths-- && "believeDead");
}


int main() {
    addr_t fake;
    fake.family = 1;
    fake.port = 2;
    fake.addr4 = 3;
    init_node(NID, LOC);
    assert(getLocation() == LOC);
    assert(me() == NID);
    pid_t pid = Fork();
    if (!pid) {
        const addr_t parent_addr = getNodeAddr(NID);
        shutdown_server();
        
        init_node(NID2, LOC);
        setNodeAddr(NID, &parent_addr);
        addr_msg amsg(me(), &fake);
        bool sent = send_msg(&amsg, NID);
        assert(sent);

        shutdown_server();
        return 0;
    }
    expectingDeaths++;
    struct msg* msg = next_msg();
    assert(msg->type == ADDRESS);
    assert(getNodeAddr(NID2) != fake);
    assert(msg_source() == NID2);
    handle_msg(msg);
    free(msg);
    assert(getNodeAddr(NID2) == fake);

    int status;
    pid_t finished = Wait(&status);
    assert(pid == finished);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);

    assert(next_msg_now() == NULL);
    shutdown_server();
    assert(expectingDeaths == 0);

    return 0;
}
