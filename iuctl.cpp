#include "iuctl.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#define HELP "help"
#define ADDR "addr"
#define NET "net"
#define STATUS "status"
#define SHUTDOWN "shutdown"


const char* help = "\
iuctl [command] [params...]\n\
    " HELP "\n\
        print this message\n\
    " ADDR " nid addr\n\
        tell server about the address of a node\n\
    " NET "\n\
        print network addresses\n\
    " STATUS "\n\
        print status\n\
    " SHUTDOWN "\n\
        shutdown iu server\n\
";
const char* addr_help = "\
iuctl addr nid addr\n\
    nid\n\
        node id\n\
    addr\n\
        IPv6 address of node\n\
";
void print_help(void) {
    fputs(help, stderr);
}
void print_addr_help(void) {
    fputs(addr_help, stderr);
}
void print_status() {
    status_iuctl();
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        exit(EINVAL);
    }
    #define IS(index,str) (strcmp(argv[index], str) == 0)
    if (IS(1, HELP)) {
        print_help();
        exit(EXIT_SUCCESS);
    }
    /* Check if server is running */
    /* TODO */
    //init_iuctl();
    if(join_iuctl() != 0) {
        fprintf(stderr, "Error, is server running?\n");
        return -1;
    }
    if (IS(1, STATUS)){
        print_status();
    } else if (IS(1, ADDR)) {
        if (argc < 4) {
            print_addr_help();
        }
        addr_t addr;
        if (inet_pton(AF_INET6, argv[3], &addr.siaddr6) == 0) {
            perror(argv[3]);
        }
        addr_iuctl(atol(argv[2]), &addr);
    } else if (IS(1, NET)) {
        net_iuctl();
    } else if (IS(1, SHUTDOWN)) {
        shutdown_iuctl();
    } else {
        print_help();
    }
    #undef IS
    return 0;
}
