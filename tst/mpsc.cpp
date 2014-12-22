#include "mpsc.h"

#include <assert.h>
#include <pthread.h>

#include <map>
using std::map;


void basic_test() {
    mpsc<int> q;
    assert(NULL == q.get());
    q.put(1);
    assert(1 == *q.get());
    assert(NULL == q.get());
    q.put(2);
    q.put(3);
    assert(2 == *q.get());
    assert(3 == *q.get());
    assert(NULL == q.get());
}

#define NITEMS 300
void* para_thread(void* arg) {
    mpsc<int>* q = (mpsc<int>*) arg;
    for (int i = 1; i <= NITEMS; i++) {
        q->put(i);
    }
}
#define NTHREADS 16
void para_test() {
    mpsc<int> q;
    pthread_t threads[NTHREADS];
    for (size_t i = 0; i < NTHREADS; i++) {
        Pthread_create(&threads[i], NULL, para_thread, &q);
    }
    map<int, size_t> counts;
    while (1) {
        const int* got = q.get();
        if (got) {
            int val = *got;
            counts[val]++;
            if (val == NITEMS && counts[val] == NTHREADS) {
                break;
            }
        } else {
            sched_yield();
        }
    }
    for (size_t i = 0; i < NTHREADS; i++) {
        Pthread_join(threads[i], NULL);
    }
    for (auto pair : counts) {
        assert(pair.second == NTHREADS);
    }
    assert(counts.size() == NITEMS);
}
int main() {
    basic_test();
    #define NTRIALS 50000
    for (size_t i = 0; i < NTRIALS; i++) {
        printf("%04X / %04X\b\b\b\b\b\b\b\b\b\b\b", i, NTRIALS);
        para_test();
    }
    printf("              \b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}
