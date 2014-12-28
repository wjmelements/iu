#include "file.h"

#define CHUNK_SIZE 1024

#define TEMPDIR "/tmp"

struct param {
    int fd;
    stream<msg*>* out;
    param(int _fd, stream<msg*>* _out):
        fd(_fd), out(_out)
    { };
};
void* send_file_thread(void* arg) {
    struct param* param = (struct param*) arg;
    int fd = param->fd;
    stream<msg*>* out = param->out;
    delete param;
    struct stat info;
    Fstat(fd, &info);
    const size_t file_size = info.st_size;
    const size_t remainder = file_size % CHUNK_SIZE;
    size_t num_full_chunks = file_size / CHUNK_SIZE;
    size_t num_chunks = num_full_chunks + (
        remainder
        ? 1
        : 0
    );
    file_header* header = (file_header*) Malloc(sizeof(file_header));
    new (header) file_header(num_chunks);
    out->put(header);
    for (size_t i = 0; i < num_full_chunks; i++) {
        file_chunk* chunk = new_file_chunk(CHUNK_SIZE, fd);
        out->put(chunk);
    }
    file_chunk* last;
    if (!remainder) {
        goto close;
    }
    last = new_file_chunk(remainder, fd);
    out->put(last);
    close:
    out->close();
    // FIXME when can I delete out?
    Close(fd);
    return NULL;
}
void send_file(int fd, nid_t dest) {
    stream<msg*>* out = send_stream(dest);
    struct param* arg = new param(Dup(fd), out);
    pthread_t thread;
    Pthread_create(&thread, NULL, send_file_thread, arg);
    Pthread_detach(thread);
}
struct finisher_arg {
    nid_t source;
};
void recv_file_finisher(void* farg) {
    struct finisher_arg* arg = (finisher_arg*) farg;
    nid_t source = arg->source;
    free(arg);

    net_resume(source);
}
struct rcv_arg {
    nid_t source;
    func_t callback;
    void* arg;
};
void* recv_file_deputy(void* darg) {
    struct rcv_arg* rarg = (rcv_arg*) darg;
    nid_t source = rarg->source;
    func_t callback = rarg->callback;
    void* arg = rarg->arg;
    free(rarg);

    int fd = Open(TEMPDIR, O_RDWR | O_TMPFILE, 0644);
    struct msg* header = next_msg_from(source);
    const struct file_header* fheader = (struct file_header*) header;
    size_t num_chunks = fheader->num_chunks;
    free(header);
    for (size_t i = 0; i < num_chunks; i++) {
        struct file_chunk* fchunk = (struct file_chunk*) next_msg_from(source);
        Write(fd, &fchunk->bytes, fchunk->getChunkSize());
        free(fchunk);
    }
    Lseek(fd, 0, SEEK_SET);

    struct finisher_arg* finarg = (finisher_arg*) Malloc(sizeof(finisher_arg));
    finarg->source = source;
    add_todo(recv_file_finisher, finarg);

    struct ffarg_t* farg = (struct ffarg_t*) Malloc(sizeof(ffarg_t));
    farg->fd = fd;
    farg->arg = arg;
    add_todo(callback, farg);
}
void recv_file(func_t callback, void* arg) {
    nid_t source = msg_source();
    net_suspend(source);

    struct rcv_arg* rarg = (rcv_arg*) Malloc(sizeof(rcv_arg));
    rarg->source = source;
    rarg->callback = callback;
    rarg->arg = arg;

    pthread_t child;
    Pthread_create(&child, NULL, recv_file_deputy, rarg);
    Pthread_detach(child);
}
char* save_file(int fd) {
    // the current path of the tmpfile is /proc/self/fd/{item.fd}
    char* oldpath = (char*) Malloc(PATH_MAX);
    Snprintf(oldpath, PATH_MAX, "/proc/self/fd/%d", fd);
    // the new path should be unique
    char* newpath = (char*) Malloc(PATH_MAX);
    union {
        struct timeval tv;
        uint64_t numeric;
    };
    Gettimeofday(&tv);
    Snprintf(newpath, PATH_MAX, TEMPDIR "/iu-%lu-%d", numeric, fd);
    Linkat(
        AT_FDCWD,
        oldpath, 
        AT_FDCWD,
        newpath,
        AT_SYMLINK_FOLLOW
    );
    free(oldpath);
    return newpath;
}
