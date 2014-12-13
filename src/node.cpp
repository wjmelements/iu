#include "node.h"
static nid_t _me;
nid_t me() {
    return _me;
}
void init_node(nid_t nid) {
    _me = nid;
    init_server();
}
