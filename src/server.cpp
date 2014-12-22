#include "iuctl.h"
#include "node.h"
#include "server.h"

bool isServer() {
    return true;
}
void believeDead(nid_t nid) {
    // TODO
}
void server_handle_msg(const msg* curr) {
    switch (curr->type) {
        default:
            handle_msg(curr);
            break;
    }
}
void main_loop() {
    while (1) {
        struct msg* next = next_msg_now();
        if (next != NULL) {
            server_handle_msg(next);
            free(next);
        } else {
            handle_iuctls();
            // non-critical maintenance here
        }
        // recurring maintenance here
        do_todos();
    }
}
