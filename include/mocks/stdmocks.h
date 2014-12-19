#include <assert.h>

#include "mocks/nodemocks.h"

nid_t me() {
    return 42;
}

datacenter getLocation() {
    return US_WEST;
}
