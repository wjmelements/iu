#include "types.h"

#include <string.h>

bool operator ==(const addr_t& one, const addr_t& two) {
    return memcmp(&one, &two, sizeof(addr_t)) == 0;
}
bool operator !=(const addr_t& one, const addr_t& two) {
    return memcmp(&one, &two, sizeof(addr_t)) != 0;
}
