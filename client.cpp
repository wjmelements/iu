#include "client.h"
#include "node.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void print_usage() {
    puts("USAGE: bin/client NID RID");
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        exit(EINVAL);
    }
    init_node(atoll(argv[1]), static_cast<datacenter>(atoi(argv[2])));
    main_loop();
    return 0;
}
