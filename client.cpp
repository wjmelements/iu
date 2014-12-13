#include "client.h"
#include "node.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void print_usage() {
    puts("USAGE: bin/client NID");
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        exit(EINVAL);
    }
    init_node(atoll(argv[1]));
    main_loop();
    return 0;
}
