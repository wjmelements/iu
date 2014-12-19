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


const char* help = "\
iuctl [command] [params...]\n\
    " HELP "\n\
        print this message\n\
    " STATUS "\n\
        print status\
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
    //init_iuctl();
    join_iuctl();
    #define IS(index,str) (strcmp(argv[index], str) == 0)
    if (IS(1, HELP)) {
        print_help();
    } else if (IS(1, STATUS)){
        print_status();
    } else {
        print_help();
    }
    destroy_iuctl();
    #undef IS
    return 0;
}
