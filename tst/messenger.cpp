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
    string libpath("lib/messenger.o");
    pid_t pid = Fork();
    if (!pid) {
        shutdown_server();

        init_node(NID2, US_WEST);
        messenger_text(hello, NID);
        assert(get_conversation(NID).size() == 1);
        assert(get_conversation(NID).begin()->type == ITEM_TEXT);

        messenger_file(libpath.c_str(), NID);
        assert(get_conversation(NID).size() == 2);
        assert(get_conversation(NID).begin()->type == ITEM_TEXT);

        messenger_text(hello, NID);
        assert(get_conversation(NID).size() == 3);

        struct msg* imsg = next_msg();
        assert(imsg->type == ITEM);
        handle_item_msg((const item_msg*) imsg);
        free(imsg);
        assert(get_conversation(NID).size() == 4);

        shutdown_server();
        messenger_destroy();
        return 0;
    }
    struct msg* tmsg = next_msg();
    assert(tmsg->type == ITEM);
    handle_item_msg((item_msg*) tmsg);
    free(tmsg);
    assert(get_conversation(NID2).size() == 1);

    struct msg* fmsg = next_msg();
    assert(fmsg->type == ITEM);
    handle_item_msg((item_msg*) fmsg);
    free(fmsg);

    struct msg* next;
    while (1) {
        next = next_msg_now();
        if (next != NULL) {
            break;
        } else {
            net_idle();
        }
        do_todos();
    }
    assert(next->type == ITEM);
    assert(get_conversation(NID2).size() == 2);
    handle_item_msg((item_msg*) next);
    free(next);
    assert(get_conversation(NID2).size() == 3);

    messenger_text(hello, NID2);
    assert(get_conversation(NID2).size() == 4);

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
