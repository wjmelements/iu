#include "node.h"

#include <string.h>

msg::msg(size_t _length, msg_type _type):
    length(_length),
    type(_type)
{
    
}


identity_msg::identity_msg():
    msg(sizeof(*this), IDENTITY),
    sender(me())
{
}

heartbeat_msg::heartbeat_msg():
    msg(sizeof(*this), HEARTBEAT)
{
}

string_msg::string_msg(size_t _size):
    msg(_size, STRING)
{

}
struct string_msg* new_string_msg(const string& str) {
    size_t len = sizeof(string_msg) + str.size() - 1;
    string_msg* ret = (string_msg*) malloc(len);
    new (ret) string_msg(len);
    memcpy(&ret->text, str.c_str(), str.size());
    return ret;
}
