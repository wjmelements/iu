#include "types.h"
enum datacenter {
    US_EAST,
    US_WEST,
    NUM_DATACENTERS

};
static inline datacenter operator++(datacenter& self) {
    return (self = (datacenter) ((unsigned) self + 1));
}
const set<nid_t>& getNodesIn(datacenter);
void setNodeLocation(nid_t, datacenter);
datacenter getNodeLocation(nid_t);
