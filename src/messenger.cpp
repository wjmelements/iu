#include "messenger.h"

#include <string.h>
#include <unistd.h>

// each message has a sequence number unique in a sender/receiver relation
static map<nid_t, seq_t> sequence_numbers; 
static void save_item(item_t& item) {
    // make a file permanent
    // assumptions: item.type == ITEM_FILE and item.saved == false
    
    item.path = save_file(item.fd);
    item.saved = true;
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
static void add_received(nid_t sender, item_t& item) {
    // FIXME can we detect dupe here instead?
    // FIXME free stuff on dupe
    received[sender][item.index] = item;
    list<item_t>& items = messages[sender];
    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->received) {
            if (item.index < it->index) {
                items.insert(it, item);
                return;
            }
            if (item.index == it->index) {
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
    item_msg item_file(ITEM_FILE, dest, sequence_numbers[dest]++);
    bool success = send_msg(&item_file, dest);
    while (!success) {
        dest = getServer();
        if (dest == (nid_t) -1) {
            // what to do
        }
        success = send_msg(&item_file, dest);
    }
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
    item_msg item_text(ITEM_TEXT, dest, sequence_numbers[dest]++);
    string_msg* str = new_string_msg(text);
    bool success = send_msg(&item_text, dest);
    while (!success) {
        dest = getServer();
        if (dest == (nid_t) -1) {
            // what to do
        }
        success = send_msg(&item_text, dest);
    }
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
struct sfarg_t {
    nid_t sender;
    item_t* item;
};
void set_finished(void* arg) {
    struct ffarg_t* farg = (ffarg_t*) arg;
    sfarg_t* sfarg = (sfarg_t*) farg->arg;
    item_t* item = sfarg->item;
    item->fd = farg->fd;
    free(farg);
    add_received(sfarg->sender, *item);
    free(item);
    free(sfarg);
}
void handle_item_msg(const item_msg* imsg) {
    if (imsg->itype == ITEM_FILE) {
        sfarg_t* sfarg = (sfarg_t*) Malloc(sizeof(sfarg_t));
        sfarg->sender = imsg->sender;
        item_t* item = sfarg->item = (item_t*) Malloc(sizeof(item_t));
        item->type = imsg->itype;
        item->index = imsg->index;
        item->saved = false;
        recv_file(set_finished, sfarg);
    } else {
        item_t item;
        item.type = imsg->itype;
        item.index = imsg->index;
        msg* smsg = next_msg_same();
        const string_msg* str = (const string_msg*) smsg;
        size_t size = str->text_size();
        item.text = (char*) Malloc(size + 1);
        memcpy(item.text, &str->text, size);
        item.text[size] = '\0';
        free(smsg);
        add_received(imsg->sender, item);
    }
}
void messenger_destroy() {
    for (auto it = messages.begin(); it != messages.end(); it++) {
        auto items = it->second;
        for (auto item = items.begin(); item != items.end(); item++) {
            if (item->type == ITEM_TEXT) {
                free(item->text);
            } else {
                if (item->saved) {
                    // do not care if this fails
                    unlink(item->path);
                    free(item->path);
                }
            }
        }
    }
    messages.clear();
    sent.clear();
    received.clear();
    sequence_numbers.clear();
}
