#ifndef PAXOS_NET
#define PAXOS_NET

#include "capitalC.h"
#include "msg.h"
#include "concat.h"
#include "types.h"

addr_t init_server(port_t port = 0);
void setNodeAddr(nid_t nid, const addr_t* addr); 

// receive stream in which to add messages and then close
// added messages are freed in src/net.cpp
concat<msg*>* send_stream(nid_t nid);

// does not free msg
bool send_msg(const struct msg* msg, nid_t nid);

// non-NULL pointers from next_* need to be passed to free()

// blocking
struct msg* next_msg();
// blocking, returns NULL on hangup only
struct msg* next_msg_same();
struct msg* next_msg_from(nid_t nid);
// nonblocking
struct msg* next_msg_now();
// source of last message
nid_t msg_source();

// do maintenance
void net_idle();

// do not poll from nid_t
void net_suspend(nid_t nid);
// resume polling from nid_t
void net_resume(nid_t nid);

// orderly shutdown
void shutdown_server();

#endif
