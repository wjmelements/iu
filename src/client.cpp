#include "client.h"
#include "node.h"

bool isServer() {
    return false;
}
void believeDead(nid_t nid) {
    // TODO
}
void client_handle_msg(const msg* curr) {
    switch (curr->type) {
        case ITEM:
            handle_item_msg((const item_msg*) curr);
            break;
        default:
            handle_msg(curr);
            break;
    }
}
void main_loop() {
    while (1) {
        struct msg* next = next_msg_now();
        if (next != NULL) {
            client_handle_msg(next);
            free(next);
        } else {
            // non-critical maintenance here
        }
        // recurring maintenance here
    }
}
