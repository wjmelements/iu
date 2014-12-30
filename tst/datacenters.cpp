#include "datacenters.h"

#include <assert.h>

int main() {
    assert(getNodesIn(US_WEST).size() == 0);
    assert(getNodesIn(US_EAST).size() == 0);
    for (datacenter loc = (datacenter) 0; loc < NUM_DATACENTERS; ++loc) {
        assert(getNodesIn(loc).size() == 0);
    }
    const nid_t nid1 = 1;

    setNodeLocation(nid1, US_WEST);
    assert(getNodeLocation(nid1) == US_WEST);
    assert(getNodesIn(US_WEST).size() == 1);
    assert(getNodesIn(US_WEST).count(nid1) == 1);

    setNodeLocation(nid1, US_EAST);
    assert(getNodesIn(US_EAST).size() == 1);
    assert(getNodeLocation(nid1) == US_EAST);
    assert(getNodesIn(US_WEST).size() == 0);
    assert(getNodesIn(US_EAST).count(nid1) == 1);

    setNodeLocation(nid1, US_EAST);
    assert(getNodesIn(US_EAST).size() == 1);
    assert(getNodeLocation(nid1) == US_EAST);
    assert(getNodesIn(US_WEST).size() == 0);
    assert(getNodesIn(US_EAST).count(nid1) == 1);

    return 0;
}
