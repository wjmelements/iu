#include "node.h"

#include <string.h>

msg::msg(size_t _length, msg_type _type):
    length(_length),
    type(_type)
{
    
}


identity_msg::identity_msg():
    msg(sizeof(*this), IDENTITY),
    loc(getLocation()),
    senderIsServer(isServer()),
    sender(me())
{
}

heartbeat_msg::heartbeat_msg():
    msg(sizeof(*this), HEARTBEAT)
{
}

file_header::file_header(size_t _num_chunks):
    msg(sizeof(*this), FILE_HEADER),
    num_chunks(_num_chunks)
{
    
}

file_chunk::file_chunk(size_t _len):
    msg(_len, FILE_CHUNK)
{
}
struct file_chunk* new_file_chunk(size_t buff_size, int fd) {
    size_t len = sizeof(struct file_chunk) + buff_size - sizeof(file_chunk::bytes);
    struct file_chunk* ret = (struct file_chunk*) Malloc(len);
    new (ret) file_chunk(len);
    Read(fd, &ret->bytes, buff_size);
    return ret;
}

string_msg::string_msg(size_t _size):
    msg(_size, STRING)
{

}
struct string_msg* new_string_msg(const string& str) {
    size_t len = sizeof(string_msg) + str.size() - 1;
    string_msg* ret = (string_msg*) Malloc(len);
    new (ret) string_msg(len);
    memcpy(&ret->text, str.c_str(), str.size());
    return ret;
}

addr_msg::addr_msg(nid_t _nid, addr_t* _addr):
    msg(sizeof(*this), ADDRESS),
    nid(_nid),
    addr(*_addr)
{

}
