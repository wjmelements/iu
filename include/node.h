#include "net.h"
nid_t me();
bool isServer();
datacenter getLocation();
void init_node(nid_t nid, datacenter loc);
void handle_msg(const msg*);
