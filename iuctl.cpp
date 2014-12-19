#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELP "help"
const char* help = "\
iuctl [command] [params...]\n\
    " HELP "\n\
        print this message\
";
void print_help() {
    puts(help);
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        exit(EINVAL);
    }
    #define IS(index,str) (strcmp(argv[index], str) == 0)
    if (IS(1, HELP)) {
        print_help();
    } else {
        print_help();
    }
    #undef IS
    return 0;
}
