#include "messenger.h"

#include <string.h>

// each message has a sequence number unique in a sender/receiver relation
static map<nid_t, seq_t> sequence_numbers; 
static void save_item(item_t& item) {
    // TODO
}
static map<nid_t, map<seq_t, item_t> > sent, received;
static map<nid_t, list<item_t> > messages;
const list<item_t>& get_conversation(nid_t with) {
    return messages[with];
}
static void add_sent(const item_msg* imsg, item_t& item) {
    sent[imsg->receiver][imsg->index] = item;
    messages[imsg->receiver].push_back(item);
}
static void add_received(const item_msg* imsg, item_t& item) {
    received[imsg->sender][imsg->index] = item;
    list<item_t>& items = messages[imsg->sender];
    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->received) {
            if (imsg->index < it->index) {
                items.insert(it, item);
                return;
            }
            if (imsg->index == it->index) {
                // dupe
                return;
            }
        }
    }
    items.push_back(item);
}
void messenger_file(const char* path, nid_t dest) {
    int fd = Open(path, O_RDONLY);
    messenger_file(fd, dest);
}
void messenger_file(int fd, nid_t dest) {
    // TODO relay through server on failure
    item_msg item_file(ITEM_FILE, dest, sequence_numbers[dest]++);
    send_msg(&item_file, dest);
    send_file(fd, dest);
    item_t item;
    item.type = ITEM_FILE;
    item.received = false;
    item.saved = false;
    item.fd = fd;
    add_sent(&item_file, item);
}
void messenger_text(const char* text, nid_t dest) {
    const string str(text);
    messenger_text(str, dest);
}
void messenger_text(const string& text, nid_t dest) {
    // TODO relay through server on failure
    item_msg item_text(ITEM_TEXT, dest, sequence_numbers[dest]++);
    string_msg* str = new_string_msg(text);
    send_msg(&item_text, dest);
    send_msg(str, dest);
    free(str);
    item_t item;
    item.type = ITEM_TEXT;
    item.received = false;
    const size_t size = text.size();
    item.text = (char*) malloc(size + 1);
    memcpy(item.text, text.c_str(), size);
    item.text[size] = '\0';
    add_sent(&item_text, item);
}
void handle_item_msg(const item_msg* imsg) {
    item_t item;
    item.type = imsg->itype;
    item.index = imsg->index;
    if (item.type == ITEM_FILE) {
        item.saved = false;
        item.fd = recv_file();
    } else {
        msg* smsg = next_msg_same();
        const string_msg* str = (const string_msg*) smsg;
        size_t size = str->text_size();
        item.text = (char*) malloc(size);
        memcpy(item.text, &str->text, size);
        item.text[size] = '\0';
    }
    add_received(imsg, item);
}
