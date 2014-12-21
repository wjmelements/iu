#include "file.h"

#define CHUNK_SIZE 1024

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
int recv_file(void) {
    int fd = Open("/tmp", O_RDWR | O_EXCL | O_TMPFILE);
    struct msg* header = next_msg_same();
    struct file_header* fheader = (struct file_header*) header;
    size_t num_chunks = fheader->num_chunks;
    free(fheader);
    // FIXME aio?
    for (size_t i = 0; i < num_chunks; i++) {
        struct file_chunk* fchunk = (struct file_chunk*) next_msg_same();
        Write(fd, &fchunk->bytes, fchunk->getChunkSize());
        free(fchunk);
    }
    Lseek(fd, 0, SEEK_SET);
    return fd;
}
