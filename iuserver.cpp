#include "server.h"
#include "node.h"
#include "iuctl.h"
#include "iuctl_server.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void print_usage() {
    fputs("USAGE: bin/server NID RID\n", stderr);
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        exit(EINVAL);
    }
    init_iuctl_server();
    init_node(atoll(argv[1]), static_cast<datacenter>(atol(argv[2])));
    main_loop();
    destroy_iuctl();
    return 0;
}
