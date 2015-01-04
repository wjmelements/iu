#include "relay.h"
struct ofarg_t {
    concat<msg*>* stream;
};
static void on_finished(void* varg) {
    ffarg_t* ffarg = (ffarg_t*) varg;
    ofarg_t* ofarg = (ofarg_t*) ffarg->arg;
    send_file(ffarg->fd, ofarg->stream);
    free(ffarg->arg);
    free(varg);
}

static void relay_string(nid_t dest) {
    msg* smsg = next_msg_same();
    send_msg(smsg, dest);
    free(smsg);
}
static void relay_file(nid_t dest) {
    ofarg_t* arg = (ofarg_t*) Malloc(sizeof(ofarg_t));
    arg->stream = send_stream(dest);
    recv_file(on_finished, arg);
}
void handle_item_relay(const item_msg* curr) {
    nid_t target = curr->receiver;
    if (!haveNodeAddr(target)) {
        target = getServer();
    }
    send_msg(curr, target);
    switch (curr->itype) {
        case ITEM_TEXT:
            relay_string(target);
            break;
        case ITEM_FILE:
            relay_file(target);
            break;
    }
}
