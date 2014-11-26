#include "types.h"
enum msg_type {
    IDENTITY,
    HEARTBEAT,
    GOSSIP
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
    identity_msg();
};

struct heartbeat_msg : msg {
    heartbeat_msg();
};
