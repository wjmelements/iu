#include "messenger.h"
#include "node.h"

#include "mocks/nodemocks.h"

#include <assert.h>

#define NID 42
#define NID2 43

size_t expectedDead = 0;
void believeDead(nid_t nid) {
    assert(expectedDead--);
}

int main() {
    init_node(NID, US_WEST);
    string hello("HELLO WORLD\n");
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();

        init_node(NID2, US_WEST);
        messenger_text(hello, NID);
        assert(get_conversation(NID).size() == 1);

        shutdown_server();
        messenger_destroy();
        return 0;
    }
    struct msg* tmsg = next_msg();
    assert(tmsg->type == ITEM);
    handle_item_msg((item_msg*) tmsg);
    free(tmsg);
    assert(get_conversation(NID2).size() == 1);

    int status;
    pid_t terminated = Wait(&status);
    assert(pid == terminated);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    
    expectedDead++;
    assert(next_msg_now() == NULL);
    shutdown_server();
    assert(expectedDead == 0);

    messenger_destroy();

    return 0;
}
