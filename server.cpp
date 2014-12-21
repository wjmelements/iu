#include "server.h"
#include "node.h"
#include "iuctl.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void print_usage() {
    puts("USAGE: bin/server NID RID");
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        exit(EINVAL);
    }
    init_node(atoll(argv[1]), static_cast<datacenter>(atol(argv[2])));
    join_iuctl();
    main_loop();
    return 0;
}
