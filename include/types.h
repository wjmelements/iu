#ifndef WNET_TYPES
#define WNET_TYPES

#include <netinet/in.h>
#include <stdint.h>

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
typedef uint64_t nid_t;
typedef uint64_t size_t;

#endif
