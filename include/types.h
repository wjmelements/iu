#ifndef WNET_TYPES
#define WNET_TYPES

#include <netinet/in.h>
#include <stdint.h>

#include <list>
using std::list;
#include <map>
using std::map;
#include <set>
using std::set;
#include <string>
using std::string;

typedef uint16_t port_t;
union addr_t {
    struct sockaddr saddr;
    sockaddr_in6 siaddr6;
    sockaddr_in siaddr4;
    struct {
        uint16_t family;
        uint16_t port;
        union {
            struct {
                uint32_t flow;
                struct in6_addr addr6;
                uint32_t scope;
            };
            struct {
                uint32_t addr4;
            };
        };
    };
};
bool operator ==(const addr_t&, const addr_t&);
bool operator !=(const addr_t&, const addr_t&);

typedef uint64_t nid_t;
typedef uint64_t size_t;
typedef uint64_t seq_t;
typedef void (*func_t)(void*);

enum item_type {
    ITEM_TEXT,
    ITEM_FILE
};
struct item_t {
    item_type type;
    bool received; // was it sent or received ?
    union {
        // if received
        seq_t index;
        // if not received then this area is undefined
    };
    union {
        // FILE
        struct {
            bool saved;
            union {
                // OPEN
                int fd;
                // SAVED
                char* path; // null-terminated
            };
        };
        // TEXT
        struct {
            char* text; // null-terminated
        };
    };
};

#endif
