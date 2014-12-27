#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "iuctl.h"

#define BUF_SIZE 0xF000
#define ARG_LEN 0xFF

#define IUCTL_PATH "bin/iuctl"
#define SERVER_PATH "bin/server"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct output_t {
    int retstat;
    char* out[BUF_SIZE];
} output_t;

static pid_t server_pid;

output_t* run(char* path, char** args, int nargs) {
    output_t* output = (output_t*)malloc(sizeof(*output));
    int pfd[2];
    pipe(pfd);
    pid_t pid = fork();
    if(pid == 0) {
        close(pfd[0]);
        char** argv;
        int argc = nargs + 1;
        argv = (char**)malloc(argc + 1);
        for(int i = 0; i < argc; i++) {
            argv[i] = (char*)malloc(ARG_LEN);
        }
        argv[argc] = NULL;
        strcpy(argv[0], path);
        for(int i = 1; i < argc; i++) {
            assert(args[i - 1] != NULL);
            strcpy(argv[i], args[i - 1]);
        }
        close(STDOUT);
        dup2(pfd[0], 1);
        execv(path, argv);
    } else if(pid > 0) {
        close(pfd[1]);
        waitpid(pid, &output->retstat, 0);
        read(pfd[0], output->out, BUF_SIZE);
    } else {
        perror("fork");
        return NULL;
    }
    return output;
}

void run_server(char** args, int nargs) {
    pid_t pid = fork();
    if(pid == 0) {
        char** argv;
        int argc = nargs + 1;
        argv = (char**)malloc(argc + 1);
        for(int i = 0; i < argc; i++) {
            argv[i] = (char*)malloc(ARG_LEN);
        }
        argv[argc] = NULL;
        strcpy(argv[0], SERVER_PATH);
        for(int i = 1; i < argc; i++) {
            assert(args[i - 1] != NULL);
            strcpy(argv[i], args[i - 1]);
        }
        close(STDOUT);
        execv(SERVER_PATH, argv);
    } else if(pid > 0) {
        server_pid = pid;
    } else {
        perror("fork");
    }
}

void test_status(void) {
    output_t* output;
    char** arg;
    arg = (char**)malloc(sizeof(char**));
    *arg = (char*)malloc(ARG_LEN);
    snprintf(*arg, ARG_LEN, "%s", "status");
    output = run(IUCTL_PATH, arg, 1);
    assert(output != NULL);
    assert(output->retstat == 0);
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
    // Wait for server to start (what a great solution!)
    sleep(1);
    for(int i = 0; i < SERV_ARGS; i++) {
        free(args[i]);
    }
    free(args);
    test_status();
    assert(server_pid > 0);
    kill(server_pid, SIGTERM);
    return 0;
}
