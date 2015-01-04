#include "messenger.h"
#include "relay.h"

#include <assert.h>

#include "mocks/netmocks.h"

#define TEXT "HELLO WORLD"

int main() {
    ME = NID1;
    addr_t addr1 = init_server();
    setNodeLocation(NID1, US_WEST);
    pid_t pid2 = Fork();
    if (!pid2) {
        shutdown_server();
        ME = NID2;
        init_server();
        setNodeAddr(NID1, &addr1);

        heartbeat_msg hmsg;
        bool success = send_msg(&hmsg, NID1);
        assert(success);

        msg* relayed = next_msg();
        assert(relayed->type == ITEM);
        handle_item_msg((item_msg*) relayed);
        free(relayed);
        assert(get_conversation(NID3).size() == 1);
        
        success = send_msg(&hmsg, NID1);
        assert(success);

        shutdown_server();
        exit(EXIT_SUCCESS);
    }
    msg* hmsg = next_msg();
    assert(hmsg->type == HEARTBEAT);
    free(hmsg);

    pid_t pid3 = Fork();
    if (!pid3) {
        shutdown_server();
        ME = NID3;
        init_server();
        setNodeAddr(NID1, &addr1);
        
        expected++;
        messenger_text(TEXT, NID2);
        assert(get_conversation(NID2).size() == 1);
        
        shutdown_server();
        exit(EXIT_SUCCESS);
    }
    expected++;
    int status;
    assert(Wait(&status) == pid3);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_SUCCESS);

    expected++;
    msg* msg = next_msg();
    assert(msg->type == ITEM);
    handle_item_relay((item_msg*) msg);
    free(msg);

    msg = next_msg();
    assert(msg->type == HEARTBEAT);
    assert(msg_source() == NID2);
    free(msg);

    assert(Wait(&status) == pid2);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_SUCCESS);
    return 0;
}
