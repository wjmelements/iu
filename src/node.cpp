#include "node.h"
static nid_t _me;
static datacenter _loc;
nid_t me() {
    return _me;
}
datacenter getLocation() {
    return _loc;
}
void init_node(nid_t nid, datacenter loc) {
    _me = nid;
    _loc = loc;
    init_server();
}
