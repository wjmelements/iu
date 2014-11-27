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
static size_t start;
static map<nid_t, addr_t> addresses;
port_t init_server(port_t port) {
    start = 0;
    int fd = Socket();
    addr_t uaddr;
    struct sockaddr_in& addr = uaddr.siaddr4;;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    int binded = bind(fd, (struct sockaddr*) &addr, sizeof(addr));
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
    socklen_t len = sizeof(addr);
    int got = getsockname(server_fd, (struct sockaddr*) &addr, &len);
    if (got == -1) {
        perror("getsockname");
        exit(errno);
    }
    addPollFd(fd);
    addresses[me()] = uaddr;
    return addr.sin_port;
}

static map<int, nid_t> connections;
static map<nid_t, int> outbound_connections;

struct pollfd* getPollfds() {
    for (auto it = pollfds_vector.begin(); it != pollfds_vector.end(); it++) {
        bzero(&it->revents, sizeof(it->revents));
    }
    return pollfds_vector.data();
}

void shutdown_server() {
	//printf("Shutting down server!\n");
    for (auto it : connections) {
        Close(it.first);
    }
    connections.clear();
    outbound_connections.clear();
    pollfds_vector.clear();
    Close(server_fd);
}

static int accept_connection() {
    int fd = Accept(server_fd, NULL, NULL);
    // initially do not know who connection is with
    connections.insert(std::pair<int, nid_t>(fd, -1));
    addPollFd(fd);
    return fd;
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
            perror("recv:recv_msg");
            exit(errno);
        }
    }
    // should have been caught by RDHUP
    if (bytes == 0) {
        nid_t node = connections[fd];
        connections.erase(fd);
        //printf("1 %u\n", node);
        believeDead(node);
        return NULL;
    }
    assert(bytes == sizeof(rcvd));
    //printf("message type %u received by %u, size %u\n", rcvd.type, me(), rcvd.length);
    struct msg* ret = (struct msg*) malloc(rcvd.length);
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
    assert(bytes != 0);
    assert(bytes == rcvd.length);
    return ret;
}

bool send_msg(const struct msg* to_send, nid_t nid) {
    auto iterator = outbound_connections.find(nid);
    int fd;
    if (iterator == outbound_connections.end()) {
        fd = Socket();
        auto address = addresses.find(nid);
        if (address == addresses.end()) {
            fprintf(stderr, "No known address for node %u\n", nid);
            return false;
        }
        addr_t addr = address->second;
        int connected = connect(fd, (struct sockaddr*) &addr, sizeof(addr));
        if (connected == -1) {
            Close(fd);
            if (errno == ECONNREFUSED) {
                believeDead(nid);
                return false;
            }
            perror("connect");
            exit(errno);
        }
        outbound_connections.insert(pair<nid_t, int>(nid, fd));
    } else {
        fd = iterator->second;
    }
    struct pollfd pollfd;
    pollfd.fd = fd;
    pollfd.events = POLLHUP;
    pollfd.revents = 0;
    int polld = poll(&pollfd, 1, 0);
    if (polld == 1) {
        if (pollfd.revents & POLLHUP) {
            //printf("POLLHUP\n");
            Close(fd);
            outbound_connections.erase(iterator);
            //printf("3 %u\n", nid);
            believeDead(nid);
            return false;

        }
    }
    ssize_t sent = send(fd, to_send, to_send->length, 0);
    if (sent == -1) {
        if (errno == ECONNRESET) {
            Close(fd);
            outbound_connections.erase(iterator);
            //printf("3 %u\n", nid);
            believeDead(nid);
            return false;
        }
        perror("send");
        exit(errno);
    }
    assert(sent == to_send->length);
    return true;
}

struct msg* next_msg_now() {
    struct pollfd* poll_fds = getPollfds();
    for (size_t offset = 0; offset < pollfds_vector.size(); offset++) {
        assert(!poll_fds[offset].revents);
    }
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
        assert(i < pollfds_vector.size());
        if (poll_fds[i].revents & POLLIN) {
            start = i;
            break;
        }
    }
    int fd;
    if (poll_fds[start].fd == server_fd) {
        fd = accept_connection();
    } else {
        fd = poll_fds[start].fd;
        if (poll_fds[start].revents & POLLRDHUP) {
            nid_t peer = connections[fd];
            // peer has closed their write end
            if (peer != -1) {
                believeDead(peer);
            }

            int fd = poll_fds[start].fd;
            Close(fd);
            connections.erase(fd);
            // do not care about events on this fd anymore
            poll_fds[start].fd = -fd;
            return next_msg_now();
        }
        assert(poll_fds[start].revents & POLLIN);
    }
    struct msg* ret = recv_msg(fd);
    if (ret == NULL) {
        return next_msg_now();
    }
    return ret;
}

struct msg* next_msg() {
    struct msg* ret;
    while (1) {
        ret = next_msg_now();
        if (ret) {
            return ret;
        } else {
            sched_yield();
        }
    }
}
