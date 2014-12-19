#include "file.h"

#define CHUNK_SIZE 1024

void send_file(int fd, nid_t dest) {
    struct stat info;
    Fstat(fd, &info);
    const size_t file_size = info.st_size;
    const size_t remainder = file_size % CHUNK_SIZE;
    size_t num_chunks = file_size / CHUNK_SIZE + (
        remainder
        ? 1
        : 0
    );
    file_header header(num_chunks);
    send_msg(&header, dest);
    for (size_t i = 0; i < file_size / CHUNK_SIZE; i++) {
        file_chunk* chunk = new_file_chunk(CHUNK_SIZE, fd);
        send_msg(chunk, dest);
        free(chunk);
    }
    if (!remainder) {
        return;
    }
    file_chunk* last = new_file_chunk(remainder, fd);
    send_msg(last, dest);
    free(last);
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
