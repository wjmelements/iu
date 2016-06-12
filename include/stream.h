/*
 * This library provides a dataflow programming framework through message
 * broadcasting to a number of listeners.
 * Copyright (C) 2013-2014 William Morriss
 */
#ifndef scion_stream_h
#define scion_stream_h
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef NULL
#define NULL 0
#endif

template <typename T> struct key;
template <typename T> class stream;
template <typename T> struct given {
    pthread_t factory; // asyncronous populate work
    stream<T>* proxy; // created by thread
};
template <typename T> struct node {
    node<T>* next;
    union {
        T data;
        size_t id;
    };
    given<T>* other;
};
template <typename T> class stream {

    protected:
        // pointer to the first node
        node<T>* first;
        // pointer to the last pointer
        node<T>** last;
        // array of pointers to volatile pointers, duh
        node<T>* volatile** listeners;
        // maximum available listener ports
        size_t max_ports;
        // are we closed?
        bool off;
        // sweep up consumed data
        void sweep();

    public:
        // responsibility of thread creator
        stream(size_t max_listeners);
        ~stream();
        struct key<T>* listen();
        // listener activities
        void cancel(size_t id);
        T get(size_t id);
        T peek(size_t id);
        void skip(size_t id);
        bool ready(size_t id);
        bool closed();
        bool depleted(size_t id);
        bool canceled(size_t id);
        bool empty(size_t id);
        // broadcaster activies
        void put(const T data);
        void give(const key<T>* other);
        void close();
        // disable copy
        stream(const stream&) = delete;
        stream& operator= (const stream&) = delete;
};
template <typename T> struct key {
    stream<T>* source;
    size_t id;
};
// possibly blocking call that gets the next element
template <typename T> T from(const key<T>* access) {
    return access->source->get(access->id);
}
// non-blocking call that checks the immediate availability of the next element
template <typename T> bool ready(const key<T>* access) {
    return access->source->ready(access->id);
}
// non-blocking call that checks the immediate openness of the stream
template <typename T> bool closed(const key<T>* access) {
    return access->source->closed();
}
// non-blocking call that checks if the key is canceled
template <typename T> bool canceled(const key<T>* access) {
    return access->source->canceled(access->id);
}
// possibly blocking call that checks whether there remain any things to get
template <typename T> bool depleted(const key<T>* access) {
    return access->source->depleted(access->id);
}
// non-blocking call that checks if there are any elements or given streams
template <typename T> bool empty(const key<T>* access) {
    return access->source->empty(access->id);
}
// cancel subscription to the stream
template <typename T> void cancel(const key<T>* access) {
    access->source->cancel(access->id);
}
// possibly blocking call that skips next element
template <typename T> void skip(const key<T>* access) {
    access->source->skip(access->id);
}
// possible blocking call that peeks at next element
template <typename T> T peek(const key<T>* access) {
    return access->source->peek(access->id);
}
/*
    The responsibility of freeing keys lies with the caller of listen, regardless of whether they give it.
    The responsibility of freeing the populate_args passed to populate lies with populate.
    The responsibility of deleting the created stream lies with this instance and is done when its node is freed.
    The responsibility of joined the created thread lies with this instance and is done when its node is freed.
*/
template<typename T> stream<T>::stream(size_t max_listeners) {
    off = false;
    first = NULL;
    last = &first;
    max_ports = max_listeners;
    listeners = (node<T>* volatile**) calloc(max_ports, sizeof(node<T>*));
}
template<typename T> stream<T>::~stream() {
    free(listeners);
    node<T>* current = first;
    while (current) {
        node<T>* remove = current;
        current = current->next;
        if (remove->other) {
            if (this->max_ports > 1) {
                pthread_cancel(remove->other->factory);
                pthread_join(remove->other->factory, NULL);
                delete remove->other->proxy;
            }
            free(remove->other);
        }
        free(remove);
    }
}
template<typename T> key<T>* stream<T>::listen() {
    for (size_t id = 0; id < max_ports; ++id) {
        if (listeners[id] == NULL) {
            listeners[id] = &first;
            struct key<T>* ret = (struct key<T>*) malloc(sizeof(key<T>));
            ret->id = id;
            ret->source = this;
            return ret;
        }
    }
    return NULL;
}
template<typename T> T stream<T>::get(size_t id) {
    node<T>* volatile* listener = listeners[id];
    if (listener == NULL) {
        exit(ECANCELED);
    }
    node<T>* next = *listener;
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
    given<T>* other = next->other;
    if (other != NULL) {
        size_t proxy_id = max_ports <= 1 ? next->id : id;
        if (!other->proxy->depleted(proxy_id)) {
            return other->proxy->get(proxy_id);
        } else {
            // then it is closed and we are good to move on
            listeners[id] = &next->next;
            return this->get(id);
        }
    }
    T ret = next->data;
    listeners[id] = &next->next;
    return ret;
}
template<typename T> T stream<T>::peek(size_t id) {
    node<T>* volatile* listener = listeners[id];
    if (listener == NULL) {
        exit(ECANCELED);
    }
    node<T>* next = *listener;
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
            sleep(0);
            next = *listener;
        }
    }
    given<T>* other = next->other;
    if (other != NULL) {
        size_t proxy_id = max_ports <= 1 ? next->id : id;
        if (!other->proxy->depleted(proxy_id)) {
            return other->proxy->peek(id);
        } else {
            // then it is closed and we are good to move on
            listeners[id] = &next->next;
            return this->peek(id);
        }
    }
    T ret = next->data;
    return ret;
}
template<typename T> void stream<T>::skip(size_t id) {
    node<T>* volatile* listener = listeners[id];
    if (listener == NULL) {
        exit(ECANCELED);
    }
    node<T>* next = *listener;
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
            sleep(0);
            next = *listener;
        }
    }
    given<T>* other = next->other;
    if (other != NULL) {
        size_t proxy_id = max_ports <= 1 ? next->id : id;
        if (other->proxy->depleted(proxy_id)) {
            // then it is closed and we are good to move on
            listeners[id] = &next->next;
            return this->skip(id);
        } else {
            return other->proxy->skip(proxy_id);
        }
    } else {
        listeners[id] = &next->next;
    }
}
template<typename T> bool stream<T>::closed() {
    return off;
}
template<typename T> bool stream<T>::empty(size_t id) {
    node<T> *const node = *listeners[id];
    if (node == NULL) {
        return true;
    }
    if (!node->other) {
        return false;
    }
    bool closed = node->other->proxy->closed();
    if (!closed) {
        return false;
    }
    if (node->other->proxy->empty(node->id)) {
        listeners[id] = &node->next;
        return empty(id);
    }
    return false;
}
template<typename T> bool stream<T>::ready(size_t id) {
    node<T> *const node = *listeners[id];
    if (node == NULL) {
        return false;
    }
    if (!node->other) {
        return true;
    }
    bool closed = node->other->proxy->closed();
    bool ready = node->other->proxy->ready(node->id);
    if (ready) {
        return true;
    }
    if (!closed) {
        return false;
    }
    listeners[id] = &node->next;
    return this->ready(id);
}
template<typename T> bool stream<T>::canceled(size_t id) {
    return listeners[id] == NULL;
}
template<typename T> bool stream<T>::depleted(size_t id) {
    node<T>* volatile* listener = listeners[id];
    while (*listener == NULL) {
        if (off) {
            if (*listener == NULL) {
                return true;
            }
        } else {
            sleep(0);
        }
    }
    node<T>* next = *listener;
    if (next->other) {
        size_t proxy_id = max_ports <= 1 ? next->id : id;
        if (next->other->proxy->depleted(proxy_id)) {
            listeners[id] = &next->next;
            return this->depleted(id);
        } else {
            return false;
        }
    }
    return false;
}
template<typename T> void stream<T>::put(const T data) {
    node<T>* put = (node<T>*) malloc(sizeof(node<T>));
    put->next = NULL;
    put->data = data;
    put->other = NULL;
    *last = put;
    last = &put->next;
    sweep();
}
template<typename T> void stream<T>::sweep() {
    node<T>* iter = first;
    // it is impossible for iter to be NULL because we have just added something
    for (size_t id = 0; id < max_ports; ++id) {
        node<T>*volatile *const listener = listeners[id];
        if (listener) {
            if (*listener == iter) {
                return;
            }
        }
    }
    iter = iter->next;
    while (iter) {
        for (size_t id = 0; id < max_ports; ++id) {
            node<T>*volatile *const listener = listeners[id];
            if (listener) {
                if (*listener == iter) {
                    return;
                }
            }
        }
        node<T>* prev = first;
        first = first->next;
        if (prev->other) {
            if (this->max_ports > 1) {
                pthread_cancel(prev->other->factory);
                pthread_join(prev->other->factory, NULL);
                delete prev->other->proxy;
            }
            free(prev->other);
        }
        free(prev);
        iter = iter->next;
    }
}
template <typename T> struct populate_args {
    const key<T>* source;
    stream<T>* destination;
};
template <typename T> void populate_cleanup(void* arg) {
    populate_args<T>* args = (populate_args<T>*) arg;
    cancel(args->source);
    free(args);
}
template <typename T> void* populate(void* arg) {
    populate_args<T>* args = (populate_args<T>*) arg;
    const key<T>* source = args->source;
    pthread_cleanup_push(populate_cleanup<T>, arg);
    stream<T>* destination = args->destination;
    while (!depleted(source)) {
        destination->put(from(source));
    }
    destination->close();
    pthread_cleanup_pop(1);
    return EXIT_SUCCESS;
}
template<typename T> void stream<T>::give(const key<T>* other) {
    if (other->source == this) {
        // prevent stack overflow recursion cases in get
        return;
    }
    node<T>* put = (node<T>*) malloc(sizeof(node<T>));
    put->next = NULL;
    put->other = (given<T>*) malloc(sizeof(given<T>));
    if (this->max_ports <= 1) {
        // then we can simply use the key
        put->other->proxy = other->source;
        put->id = other->id;
    } else {
        // we need to propogate this key to all our listeners
        put->other->proxy = new stream<T>(max_ports);
        stream<T>* proxy = put->other->proxy;
        for (int i = 0; i < max_ports; i++) {
            proxy->listeners[i] = &proxy->first;
        }
        populate_args<T>* args = (populate_args<T>*) malloc(sizeof(populate_args<T>));
        args->source = other;
        args->destination = proxy;
        pthread_create(&put->other->factory, NULL, populate<T>, args);
    }
    *last = put;
    last = &put->next;
    sweep();
}
template<typename T> void stream<T>::close() {
    off = true;
}
template<typename T> void stream<T>::cancel(size_t id) {
    listeners[id] = NULL;
}
#endif
