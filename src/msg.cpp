#include "msg.h"
#include "node.h"

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
