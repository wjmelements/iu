#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "iuctl.h"

#define BUF_SIZE 0xF000
#define ARG_LEN 0xFF

#define IUCTL_PATH "bin/iuctl"
#define SERVER_PATH "bin/iuserver"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct output_t {
    int retstat;
    ssize_t nbytes;
    char* out[BUF_SIZE];
} output_t;

static pid_t server_pid;

output_t* run(char* path, char** args, int nargs) {
    output_t* output = (output_t*) Malloc(sizeof(*output));
    int pfd[2];
    Pipe(pfd);
    pid_t pid = Fork();
    if(!pid) {
        Close(pfd[0]);
        char** argv;
        int argc = nargs + 1;
        argv = (char**) Malloc(sizeof(char*) * (argc + 1));
        for(int i = 0; i < argc; i++) {
            argv[i] = (char*) Malloc(ARG_LEN);
        }
        argv[argc] = NULL;
        strcpy(argv[0], path);
        for(int i = 1; i < argc; i++) {
            assert(args[i - 1] != NULL);
            strcpy(argv[i], args[i - 1]);
        }
        Close(STDOUT);
        dup2(pfd[1], STDOUT);
        execv(path, argv);
        perror("execv");
    }
    Close(pfd[1]);
    waitpid(pid, &output->retstat, 0);
    output->nbytes = read(pfd[0], output->out, BUF_SIZE);
    return output;
}

void run_server(char** args, int nargs) {
    pid_t pid = Fork();
    if(pid == 0) {
        char** argv;
        int argc = nargs + 1;
        argv = (char**) Malloc(sizeof(char*) * (argc + 1));
        for(int i = 0; i < argc; i++) {
            argv[i] = (char*) Malloc(ARG_LEN);
        }
        argv[argc] = NULL;
        strcpy(argv[0], SERVER_PATH);
        for(int i = 1; i < argc; i++) {
            assert(args[i - 1] != NULL);
            strcpy(argv[i], args[i - 1]);
        }
        execv(SERVER_PATH, argv);
    } else {
        server_pid = pid;
    }
}

void test_status(void) {
    output_t* output;
    char** arg;
    arg = (char**) Malloc(sizeof(char**));
    *arg = (char*) Malloc(ARG_LEN);
    snprintf(*arg, ARG_LEN, "%s", "status");
    output = run(IUCTL_PATH, arg, 1);
    assert(output != NULL);
    assert(output->retstat == 0);
    assert(output->nbytes > 0);
    free(*arg);
    free(arg);
    free(output);
}

void test_send_recv() {
    iuctl_msg_t msg;
}

#define SERV_ARGS 2

int main() {
    char** args;
    args = (char**)malloc(sizeof(char**) * SERV_ARGS);
    for(int i = 0; i < SERV_ARGS; i++) {
        args[i] = (char*)malloc(ARG_LEN);
    }
    snprintf(args[0], ARG_LEN, "%d", 1);
    snprintf(args[1], ARG_LEN, "%d", 1);
    run_server(args, 2);
    for(int i = 0; i < SERV_ARGS; i++) {
        free(args[i]);
    }
    free(args);
    // Wait for server to start (what a great solution!)
    // FIXME no sleep here
    sleep(1);
    test_status();
    assert(server_pid > 0);
    kill(server_pid, SIGTERM);
    return 0;
}
