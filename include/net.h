#ifndef PAXOS_NET
#define PAXOS_NET

#include "capitalC.h"
#include "msg.h"
#include "types.h"

port_t init_server(port_t port = 0);
bool send_msg(const struct msg* msg, nid_t nid);
// blocking
struct msg* next_msg();
// nonblocking
struct msg* next_msg_now();
// source of last message
nid_t msg_source();

// orderly shutdown
void shutdown_server();

#endif
