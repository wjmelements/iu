#include "net.h"
#include "todo.h"
void send_file(int fd, nid_t dest);

struct ffarg_t {
    int fd;
    void* arg;
};

void recv_file(func_t callback, void* arg);
// returns the path; must be freed
char* save_file(int fd);
