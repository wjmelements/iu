#include "capitalC.h"
#include "iuctl.h"
#include <sys/types.h>

/* Message specific handlers */
void iuctl_server_status(pid_t iuctl_pid);
void iuctl_server_shutdown(pid_t iuctl_pid);

// called by only server
void handle_iuctls();
