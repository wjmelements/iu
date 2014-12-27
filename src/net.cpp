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
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
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
    pollfds_vector.erase(
        remove_if(pollfds_vector.begin(), pollfds_vector.end(), [](const struct pollfd& pollfd) {
            return pollfd.fd < 0;
        }),
        pollfds_vector.end()
    );
}
static size_t start; // for round-robin
static map<nid_t, addr_t> addresses;
addr_t init_server(port_t port) {
    start = 0;
    int fd = Socket();
    addr_t uaddr;
    bzero(&uaddr, sizeof(uaddr));
    struct sockaddr_in6& addr = uaddr.siaddr6;;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = port;
    socklen_t len = sizeof(uaddr);
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
    return uaddr;
}

void setNodeAddr(nid_t nid, const addr_t* addr) {
    // TODO emplace
    addresses[nid] = *addr;
}
// used primarily for unit testing
const addr_t& getNodeAddr(nid_t nid) {
    auto it = addresses.find(nid);
    if (it == addresses.end()) {
        assert(0 && "getNodeaddr");
    }
    return it->second;
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

void net_suspend(nid_t nid) {
    // FIXME better efficiency here
    auto nit = connections.find(nid);
    if (nit == connections.end()) {
        return;
    }
    int fd = nit->second;
    for (auto it = pollfds_vector.begin(); it != pollfds_vector.end(); it++) {
        if (it->fd == fd) {
            it->fd = -fd;
            return;
        }
    };
}
void net_resume(nid_t nid) {
    auto nit = connections.find(nid);
    if (nit == connections.end()) {
        return;
    }
    int fd = nit->second;
    addPollFd(fd);
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
        auto nidi = nids.find(fd);
        if (nidi == nids.end()) {
            // this shouldn't happen
            fprintf(stderr, "unexpected fd %i\n", fd);
            for (auto pollfd : pollfds_vector) {
                printf("item: %i\n", pollfd.fd);
            }
            return NULL;
        }
        nid_t nid = nidi->second;
        nids.erase(fd);
        connections.erase(nid);
        //printf("1 %u\n", nid);
        believeDead(nid);
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

struct msg* next_msg_from_fd(int fd) {
    struct pollfd pollfd;
    while (1) {
        pollfd.fd = fd;
        pollfd.events = POLLIN | POLLRDHUP;
        pollfd.revents = 0;
        Poll(&pollfd, 1, 0);
        if (!pollfd.revents) {
            net_idle();
            sched_yield();
        } else if (pollfd.revents & POLLIN) {
            return recv_msg(fd);
        } else if (pollfd.revents & POLLRDHUP) {
            auto nidi = nids.find(fd);
            if (nidi != nids.end()) {
                believeDead(nidi->second);
            }
            return NULL;
        }
    }
}

static void accept_connection() {
    addr_t addr;
    socklen_t len = sizeof(addr);
    int fd = Accept(server_fd, &addr.saddr, &len);
    struct msg* msg = next_msg_from_fd(fd);
    if (msg == NULL) {
        Close(fd);
        return;
    }
    struct identity_msg* id_msg = (struct identity_msg*) msg;
    nid_t nid = id_msg->sender;
    free(id_msg);
    setNodeAddr(nid, &addr);
    nids.insert(std::pair<int, nid_t>(fd, nid));
    connections.insert(std::pair<nid_t, int>(nid, fd));
    stream<struct msg*>* const strm = send_qs[nid] = new stream<struct msg*>(/*listeners*/ 1);
    send_keys[nid] = strm->listen();
    addPollFd(fd);
    return;
}

struct msg* next_msg_same() {
    return next_msg_from_fd(last_fd);
}
struct msg* next_msg_from(nid_t nid) {
    return next_msg_from_fd(connections.find(nid)->second);
}

static bool send_msg_now(const struct msg* to_send, nid_t nid);

static int connect_to(nid_t nid) {
    int fd = Socket();
    auto address = addresses.find(nid);
    if (address == addresses.end()) {
        fprintf(stderr, "No known address for node %u\n", nid);
        return -1;
    }
    const addr_t addr = address->second;
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
    nids.insert(pair<int, nid_t>(fd, nid));
    connections.insert(pair<nid_t, int>(nid, fd));
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
    struct msg* const copy = (struct msg*) Malloc(msg->length);
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
    // start can lie outside of the actual range
    while (start >= pollfds_vector.size()) {
        start -= pollfds_vector.size();
    }
    // round robin
    for (size_t offset = 0; offset < pollfds_vector.size(); offset++) {
        size_t i = offset + start + 1;
        i = i >= pollfds_vector.size() ? i - pollfds_vector.size() : i;
        if ((poll_fds[i].revents & POLLIN) || (poll_fds[i].revents & POLLRDHUP)) {
            start = i;
            break;
        }
    }
    if (poll_fds[start].fd == server_fd) {
        accept_connection();
        return next_msg_now();
    }
    int fd = poll_fds[start].fd;
    bool negate_on_null = false;
    if (poll_fds[start].revents & POLLRDHUP) {
        // peer has closed their write end
        // still have to receive last messages
        negate_on_null = true;
    }
    struct msg* ret;
    if (
        !(poll_fds[start].revents & POLLIN)
     || (ret = recv_msg(fd)) == NULL
    ) {
        if (negate_on_null) {
            // do not care about events on this fd anymore
            poll_fds[start].fd = -fd;
        }
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
