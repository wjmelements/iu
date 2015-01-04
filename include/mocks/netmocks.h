#include "mocks/stdmocks.h"
const nid_t NID1 = 42;
const nid_t NID2 = 43;
const nid_t NID3 = 44;
nid_t ME;
nid_t me() {
    return ME;
}

int expected = 0;
void believeDead(nid_t nid) {
    assert(expected --> 0);
}

