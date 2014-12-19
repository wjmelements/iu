#include "file.h"
// send
void messenger_text(const char* text, nid_t dest);
void messenger_text(const string& text, nid_t dest);
void messenger_file(const char* path, nid_t dest);
void messenger_file(int fd, nid_t dest);

// receive
void handle_item_msg(const item_msg*);

// view conversation
const list<item_t>& get_conversation(nid_t with);

// cleanup
void messenger_destroy();
