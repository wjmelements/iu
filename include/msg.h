#include "datacenters.h"
#include "types.h"
enum msg_type {
    IDENTITY,
    HEARTBEAT,
    ADDRESS,
    // client to client (maybe through server)
    ITEM,
    // submessages
    FILE_HEADER,
    FILE_CHUNK,
    STRING
};
struct msg {
    size_t length;
    enum msg_type type;

    msg(){};
protected:
    msg(size_t length, msg_type type);
};

struct item_msg : msg {
    nid_t sender;
    nid_t receiver;
    seq_t index;
    item_type itype;

    item_msg(item_type, nid_t receiver, seq_t index);
};

struct identity_msg : msg {
    nid_t sender;
    datacenter loc;
    bool senderIsServer;

    identity_msg();
};

struct file_header : msg {
    size_t num_chunks;

    file_header(size_t num_chunks);
};

struct file_chunk : msg {
    char bytes;

    inline size_t getChunkSize() {
        return length - sizeof(file_chunk) + sizeof(file_chunk::bytes);
    }
    friend struct file_chunk* new_file_chunk(size_t buff_size, int fd);
private:
    file_chunk(size_t len);
};
struct file_chunk* new_file_chunk(size_t buff_size, int fd);

struct heartbeat_msg : msg {
    heartbeat_msg();
};

// variable length
struct string_msg : msg {
    // not NULL-terminated
    char text;

    inline size_t text_size() const {
        return this->length - sizeof(string_msg) + sizeof(string_msg::text);
    };
    friend struct string_msg* new_string_msg(const string& str);
private:
    string_msg(size_t len);
};
struct string_msg* new_string_msg(const string& str);

struct addr_msg : msg {
    nid_t nid;
    addr_t addr;
    bool isServer;

    addr_msg(nid_t nid, addr_t* addr);
};

