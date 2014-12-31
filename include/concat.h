/*
 * This library provides a dataflow programming framework through message
 * broadcasting to a number of listeners.
 * Copyright (C) 2013-2014 William Morriss
 */
#ifndef scion_concat_h
#define scion_concat_h
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef NULL
#define NULL 0
#endif

/* 
    concat is a stream specialized for one listener 
    also removed is cancel functionality
*/

template <typename T> class concat {
    struct node {
        node* next;
        T data;
        concat<T>* given;
    };

    protected:
        // producer pointer to the first node
        node* first;
        // producer pointer to the last pointer
        node** last;
        // consumer pointer to pointer to next to read
        node* volatile* listener;
        // are we closed?
        bool off;
        // producer sweep up consumed data
        void sweep();

    public:
        // responsiblity of thread creator
        concat();
        ~concat();
        // listener activities
        T get();
        T peek();
        void skip();
        // listener checks
        bool closed(); // non-blocking
        bool ready(); // non-blocking
        bool depleted(); // blocking
        bool empty(); // non-blocking
        // broadcaster activies
        void put(const T data);
        void give(concat<T>* other); // cannot give self
        void close();
        // disable copy
        concat(const concat&) = delete;
        concat& operator= (const concat&) = delete;
};
/*
    The responsibility of freeing the populate_args passed to populate lies with populate.
    The responsibility of deleting the given concat lies with this instance and is done when its node is freed.
    The responsibility of joined the created thread lies with this instance and is done when its node is freed.
*/
template<typename T> concat<T>::concat() {
    off = false;
    first = NULL;
    listener = last = &first;
}
template<typename T> concat<T>::~concat() {
    node* current = first;
    while (current) {
        node* remove = current;
        current = current->next;
        if (remove->given) {
            delete remove->given;
        }
        free(remove);
    }
}
template<typename T> T concat<T>::get() {
    node* next = *listener;
    while (next == NULL) {
        if (off) {
            next = *listener;
            if (next == NULL) {
                exit(ENODATA);
            }
            else {
                break;
            }
        } else {
            sched_yield();
            next = *listener;
        }
    }
    concat<T>* other = next->given;
    if (other != NULL) {
        if (!other->depleted()) {
            return other->get();
        } else {
            // then it is closed and we are good to move on
            listener = &next->next;
            return this->get();
        }
    }
    T ret = next->data;
    listener = &next->next;
    return ret;
}
template<typename T> T concat<T>::peek() {
    node* next = *listener;
    while (next == NULL) {
        if (off) {
            next = *listener;
            if (next == NULL) {
                exit(ENODATA);
            }
            else {
                break;
            }
        } else {
            sched_yield();
            next = *listener;
        }
    }
    concat<T>* other = next->given;
    if (other != NULL) {
        if (!other->depleted()) {
            return other->peek();
        } else {
            // then it is closed and we are good to move on
            listener = &next->next;
            return this->peek();
        }
    }
    return next->data;
}
template<typename T> void concat<T>::skip() {
    node* next = *listener;
    while (next == NULL) {
        if (off) {
            next = *listener;
            if (next == NULL) {
                exit(ENODATA);
            }
            else {
                break;
            }
        } else {
            sched_yield();
            next = *listener;
        }
    }
    concat<T>* other = next->given;
    if (other != NULL) {
        if (!other->depleted()) {
            return other->skip();
        } else {
            // then it is closed and we are good to move on
            listener = &next->next;
            return this->skip();
        }
    }
    listener = &next->next;
}
template<typename T> bool concat<T>::closed() {
    return off;
}
template<typename T> bool concat<T>::empty() {
    node *const node = *listener;
    if (node == NULL) {
        return true;
    }
    if (!node->given) {
        return false;
    }
    bool closed = node->given->closed();
    if (!closed) {
        return false;
    }
    if (node->given->empty()) {
        listener = &node->next;
        return this->empty();
    }
    return false;
}
template<typename T> bool concat<T>::ready() {
    node *const node = *listener;
    if (node == NULL) {
        return false;
    }
    if (!node->given) {
        return true;
    }
    bool closed = node->given->closed();
    bool ready = node->given->ready();
    if (ready) {
        return true;
    }
    if (!closed) {
        return false;
    }
    listener = &node->next;
    return this->ready();
}
template<typename T> bool concat<T>::depleted() {
    while (*listener == NULL) {
        if (off) {
            if (*listener == NULL) {
                return true;
            }
        } else {
            sched_yield();
        }
    }
    node* next = *listener;
    if (next->given) {
        if (next->given->depleted()) {
            listener = &next->next;
            return this->depleted();
        } else {
            return false;
        }
    }
    return false;
}
template<typename T> void concat<T>::put(const T data) {
    node* put = (node*) malloc(sizeof(node));
    put->next = NULL;
    put->data = data;
    put->given = NULL;
    *last = put;
    last = &put->next;
    sweep();
}
template<typename T> void concat<T>::sweep() {
    node* iter = first;
    // it is impossible for iter to be NULL because we have just added something
    if (*listener == iter) {
        return;
    }
    iter = iter->next;
    while (iter) {
        if (*listener == iter) {
            return;
        }
        node* prev = first;
        first = first->next;
        if (prev->given) {
            delete prev->given;
        }
        free(prev);
        iter = iter->next;
    }
}
template<typename T> void concat<T>::give(concat<T>* other) {
    node* put = (node*) malloc(sizeof(node));
    put->next = NULL;
    put->given = other;
    *last = put;
    last = &put->next;
    sweep();
}
template<typename T> void concat<T>::close() {
    off = true;
}
#endif
