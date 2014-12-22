#include "capitalC.h"

#include <atomic>
using std::atomic_exchange;
using std::atomic;

template <typename T> struct mpsc {
private:
    struct node {
        node* next;
        T data;
    };
    struct node*first;
    atomic<node*> last;
public:
    mpsc() {
        first = (node*) Malloc(sizeof(*first));
        last = first;
    };
    ~mpsc() {
        while (first) {
            node* to_delete = first;
            first = first->next;
            free(to_delete);
        }
    };
    void put(T item) {
        struct node* proposal = (node*) Malloc(sizeof(*proposal));
        proposal->data = item;
        proposal->next = NULL;
        struct node* prev = atomic_exchange(&last, proposal);
        prev->next = proposal;
    };
    const T* get() {
        if (!first->next) {
            return NULL;
        }
        node* to_delete = first;
        first = first->next;
        free(to_delete);
        return &first->data;
    };
};
