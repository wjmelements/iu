#include "datacenters.h"
#include "types.h"
enum msg_type {
    IDENTITY,
    HEARTBEAT,
    ADDRESS,
    // submessages
    STRING
};
struct msg {
    size_t length;
    enum msg_type type;

    msg(){};
protected:
    msg(size_t length, msg_type type);
};

struct identity_msg : msg {
    nid_t sender;
    datacenter loc;
    bool senderIsServer;

    identity_msg();
};

struct heartbeat_msg : msg {
    heartbeat_msg();
};

// variable length
struct string_msg : msg {
    // not NULL-terminated
    char text;

    friend struct string_msg* new_string_msg(const string& str);
private:
    string_msg(size_t len);
};
struct string_msg* new_string_msg(const string& str);

struct addr_msg : msg {
    nid_t nid;
    addr_t addr;
    bool isServer;

    addr_msg(nid_t nid, addr_t* addr);
};

