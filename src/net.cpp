#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "net.h"
#include "node.h"

#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#include <algorithm>
using std::remove_if;
#include <map>
using std::map;
using std::pair;
#include <set>
using std::set;
#include <vector>
using std::vector;

#define BACKLOG 500

extern void believeDead(nid_t node);

int Socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(errno);
    }
    return fd;
}

static int server_fd;
static vector<struct pollfd> pollfds_vector;
static void addPollFd(int fd) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLRDHUP;
    pollfds_vector.push_back(pfd);
}
static void prunePollFds() {
    remove_if(pollfds_vector.begin(), pollfds_vector.end(), [](const struct pollfd& pollfd) {
        return pollfd.fd < 0;
    });
}
static size_t start; // for round-robin
static map<nid_t, addr_t> addresses;
port_t init_server(port_t port) {
    start = 0;
    int fd = Socket();
    addr_t uaddr;
    struct sockaddr_in& addr = uaddr.siaddr4;;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    socklen_t len = sizeof(addr);
    int binded = bind(fd, &uaddr.saddr, len);
    if (binded == -1) {
        perror("bind");
        exit(errno);
    }
    int listening = listen(fd, BACKLOG);
    if (listening == -1) {
        perror("listen");
        exit(errno);
    }
    server_fd = fd;
    int got = getsockname(fd, &uaddr.saddr, &len);
    if (got == -1) {
        perror("getsockname");
        exit(errno);
    }
    addPollFd(fd);
    setNodeAddr(me(), &uaddr);
    return addr.sin_port;
}

void setNodeAddr(nid_t nid, const addr_t* addr) {
    addresses[nid] = *addr;
}
// used primarily for unit testing
const addr_t& getNodeAddr(nid_t nid) {
    return addresses[nid];
}

static map<int, nid_t> nids;
static map<nid_t, int> connections;
static map<nid_t, stream<msg*>* > send_qs;
static map<nid_t, key<msg*>* > send_keys;

struct pollfd* getPollfds() {
    for (auto it = pollfds_vector.begin(); it != pollfds_vector.end(); it++) {
        bzero(&it->revents, sizeof(it->revents));
    }
    return pollfds_vector.data();
}

void shutdown_server() {
	//printf("Shutting down server!\n");
    for (auto it : nids) {
        Close(it.first);
    }
    nids.clear();
    connections.clear();
    pollfds_vector.clear();
    for (auto pair : send_qs) {
        delete pair.second;
    }
    send_qs.clear();
    for (auto pair : send_keys) {
        free(pair.second);
    }
    send_keys.clear();
    Close(server_fd);
}

static int last_fd;
nid_t msg_source() {
    return nids[last_fd];
}
static struct msg* recv_msg(int fd) {
    struct msg rcvd;
    ssize_t bytes = recv(fd, &rcvd, sizeof(rcvd), MSG_PEEK);
    if (bytes == -1) {
        switch (errno) {
        case EAGAIN:
            // retry
            return NULL;
        default:
            perror("recv:recv_msg:peek");
            exit(errno);
        }
    }
    // should have been caught by RDHUP
    if (bytes == 0) {
        nid_t node = nids[fd];
        nids.erase(fd);
        //printf("1 %u\n", node);
        believeDead(node);
        return NULL;
    }
    //printf("message type %u received by %u, size %u\n", rcvd.type, me(), rcvd.length);
    struct msg* ret = (struct msg*) Malloc(rcvd.length);
    bytes = recv(fd, ret, rcvd.length, 0);
    if (bytes == -1) {
        switch (errno) {
        case EAGAIN:
            // retry
            free(ret);
            return NULL;
        default:
            perror("recv:recv_msg");
            exit(errno);
        }
    }
    return ret;
}

static int accept_connection() {
    int fd = Accept(server_fd, NULL, NULL);
    struct msg* msg = recv_msg(fd);
    struct identity_msg* id_msg = (struct identity_msg*) msg;
    nid_t nid = id_msg->sender;
    free(id_msg);
    nids.insert(std::pair<int, nid_t>(fd, nid));
    connections.insert(std::pair<nid_t, int>(nid, fd));
    addPollFd(fd);
    return fd;
}

struct msg* next_msg_same() {
    struct pollfd pollfd;
    while (1) {
        pollfd.fd = last_fd;
        pollfd.events = POLLIN | POLLRDHUP;
        pollfd.revents = 0;
        Poll(&pollfd, 1, 0);
        if (!pollfd.revents) {
            net_idle();
            sched_yield();
        } else if (pollfd.revents & POLLIN) {
            return recv_msg(last_fd);
        } else if (pollfd.revents & POLLRDHUP) {
            believeDead(nids[last_fd]);
            return NULL;
        }
    }
}

static bool send_msg_now(const struct msg* to_send, nid_t nid);

static int connect_to(nid_t nid) {
    int fd = Socket();
    auto address = addresses.find(nid);
    if (address == addresses.end()) {
        fprintf(stderr, "No known address for node %u\n", nid);
        return -1;
    }
    addr_t addr = address->second;
    int connected = connect(fd, &addr.saddr, sizeof(addr));
    if (connected == -1) {
        Close(fd);
        if (errno == ECONNREFUSED) {
            believeDead(nid);
            return -1;
        }
        perror("connect");
        exit(errno);
    }
    connections.insert(pair<nid_t, int>(nid, fd));
    nids.insert(pair<int, nid_t>(fd, nid));
    stream<msg*>* const strm = send_qs[nid] = new stream<msg*>(/*listeners*/ 1);
    send_keys[nid] = strm->listen();
    addPollFd(fd);

    identity_msg idm;
    send_msg_now(&idm, nid);

    return fd;
}

static bool send_msg_now(const struct msg* to_send, nid_t nid) {
    auto iterator = connections.find(nid);
    int fd = iterator->second;
    struct pollfd pollfd;
    pollfd.fd = fd;
    pollfd.events = POLLHUP;
    pollfd.revents = 0;
    int polld = poll(&pollfd, 1, 0);
    if (polld == 1) {
        if (pollfd.revents & POLLHUP) {
            //printf("POLLHUP\n");
            Close(fd);
            nids.erase(fd);
            connections.erase(iterator);
            //printf("3 %u\n", nid);
            believeDead(nid);
            return false;
        }
    }
    ssize_t sent = send(fd, to_send, to_send->length, 0);
    if (sent == -1) {
        if (errno == ECONNRESET) {
            Close(fd);
            nids.erase(fd);
            connections.erase(iterator);
            //printf("3 %u\n", nid);
            believeDead(nid);
            return false;
        }
        perror("send");
        exit(errno);
    }
    return true;
}

bool send_msg(const struct msg* msg, nid_t nid) {
    auto send_q = send_qs.find(nid);
    if (send_q == send_qs.end()) {
        int fd = connect_to(nid);
        if (fd == -1) {
            return false;
        }
        send_q = send_qs.find(nid);
    }
    if (empty(send_keys[nid])) {
        // can send now
        return send_msg_now(msg, nid);
    }
    // put in queue and send later
    struct msg* const copy = (struct msg*) malloc(msg->length);
    memcpy(copy, msg, msg->length);
    send_q->second->put(copy);
    return true;
}
stream<msg*>* send_stream(nid_t nid) {
    auto send_q = send_qs.find(nid);
    if (send_q == send_qs.end()) {
        int fd = connect_to(nid);
        if (fd == -1) {
            return NULL;
        }
        send_q = send_qs.find(nid);
    }
    stream<msg*>* ret = new stream<msg*>(/*listeners*/ 1);
    key<msg*>* key = ret->listen();
    send_q->second->give(key);
    free(key);
    return ret;
}

static void send_readies() {
    // TODO round-robin this
    for (auto pair : send_keys) {
        while (ready(pair.second)) {
            msg* next = from(pair.second);
            send_msg_now(next, pair.first);
            free(next);
        }
    }
}

struct msg* next_msg_now() {
    struct pollfd* poll_fds = getPollfds();
    const int timeout = 0;
    int polld = poll(poll_fds, pollfds_vector.size(), timeout);
    if (polld == -1) {
        perror("poll");
        exit(errno);
    }
    if (polld == 0) {
        return NULL;
    }
    // round robin
    for (size_t offset = 0; offset < pollfds_vector.size(); offset++) {
        size_t i = offset + start + 1;
        i = i >= pollfds_vector.size() ? i - pollfds_vector.size() : i;
        if (poll_fds[i].revents & POLLIN) {
            start = i;
            break;
        }
    }
    if (poll_fds[start].fd == server_fd) {
        accept_connection();
        return next_msg_now();
    }
    int fd = poll_fds[start].fd;
    if (poll_fds[start].revents & POLLRDHUP) {
        nid_t peer = nids[fd];
        // peer has closed their write end
        if (peer != -1) {
            believeDead(peer);
        }

        int fd = poll_fds[start].fd;
        Close(fd);
        nids.erase(fd);
        // do not care about events on this fd anymore
        poll_fds[start].fd = -fd;
        return next_msg_now();
    }
    assert(poll_fds[start].revents & POLLIN);
    struct msg* ret = recv_msg(fd);
    if (ret == NULL) {
        return next_msg_now();
    }
    last_fd = fd;
    return ret;
}

struct msg* next_msg() {
    struct msg* ret;
    while (1) {
        ret = next_msg_now();
        if (ret) {
            return ret;
        } else {
            net_idle();
            sched_yield();
        }
    }
}
void net_idle() {
    prunePollFds();
    send_readies();
}
