#include "iuctl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#define HELP "help"
#define STATUS "status"
#define SHUTDOWN "shutdown"


const char* help = "\
iuctl [command] [params...]\n\
    " HELP "\n\
        print this message\n\
    " STATUS "\n\
        print status\
    " SHUTDOWN "\n\
        shutdown iu server\
";
void print_help(void) {
    puts(help);
}
void print_status() {
    status_iuctl();
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        exit(EINVAL);
    }
    /* Check if server is running */
    /* TODO */
    //init_iuctl();
    if(join_iuctl() != 0) {
        fprintf(stderr, "Error, is server running?\n");
        return -1;
    }
    #define IS(index,str) (strcmp(argv[index], str) == 0)
    if (IS(1, HELP)) {
        print_help();
    } else if (IS(1, STATUS)){
        print_status();
    } else if (IS(1, SHUTDOWN)) {
        shutdown_iuctl();
    } else {
        print_help();
    }
    #undef IS
    return 0;
}
