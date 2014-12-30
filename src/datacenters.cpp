#include "datacenters.h"
static map<datacenter, set<nid_t> > datacenters;
static map<nid_t, datacenter> nodes;
const set<nid_t>& getNodesIn(datacenter loc) {
    return datacenters[loc];
}
void setNodeLocation(nid_t nid, datacenter loc) {
    auto pair = nodes.emplace(nid, loc);
    // the meaning of first, second  is obvious; thanks C++
    if (!pair.second) {
        datacenter& prev = pair.first->second;
        datacenters[prev].erase(nid);
        prev = loc;
    }
    datacenters[loc].emplace(nid);
}
datacenter getNodeLocation(nid_t nid) {
    return nodes[nid];
}
