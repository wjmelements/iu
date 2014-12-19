#ifndef PAXOS_NET
#define PAXOS_NET

#include "capitalC.h"
#include "msg.h"
#include "types.h"

port_t init_server(port_t port = 0);
void setNodeAddr(nid_t nid, const addr_t* addr); 
bool send_msg(const struct msg* msg, nid_t nid);

// non-NULL pointers from next_* need to be passed to free()

// blocking
struct msg* next_msg();
// blocking, returns NULL on hangup only
struct msg* next_msg_same();
// nonblocking
struct msg* next_msg_now();
// source of last message
nid_t msg_source();

// orderly shutdown
void shutdown_server();

#endif
