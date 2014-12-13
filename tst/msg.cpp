#include "msg.h"

#include <assert.h>
#include <string.h>

nid_t me() {
    return 42;
}

int main() {
    heartbeat_msg msg;
    assert(msg.type == HEARTBEAT);
    assert(msg.length == sizeof(msg));
    
    string hello("HELLO WORLD!\n");
    string_msg* str = new_string_msg(hello);
    assert(str->type == STRING);
    assert(str->length > hello.size());
    assert(0 == memcmp(&str->text, hello.c_str(), hello.size()));
    free(str);

    return 0;
}
