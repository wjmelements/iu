#ifndef IUMPSC
#define IUMPSC

#include "capitalC.h"

#include <atomic>
using std::atomic_exchange;
using std::atomic;

template <typename T> struct mpsc {
private:
    struct node {
        atomic<node*> volatile next;
        T data;
    };
    struct node* first;
    atomic<node*> volatile last;
public:
    mpsc() {
        first = (node*) Malloc(sizeof(*first));
        first->next.store(NULL);
        last.store(first);
    };
    ~mpsc() {
        while (first) {
            node* to_delete = first;
            first = first->next.load();
            free(to_delete);
        }
    };
    void put(T item) {
        register struct node *const proposal = (node*) Malloc(sizeof(*proposal));
        proposal->data = item;
        proposal->next.store(NULL);
        struct node* const prev = last.exchange(proposal);
        prev->next.store(proposal);
    };
    const T* get() {
        if (!first->next.load()) {
            return NULL;
        }
        struct node* const to_delete = first;
        first = first->next.load();
        free(to_delete);
        return &first->data;
    };
};
#endif
