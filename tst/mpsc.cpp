#include "mpsc.h"

#include <assert.h>
#include <pthread.h>

#include <map>
using std::map;
using std::pair;


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
struct worker_arg {
    int id;
    mpsc<pair<int, int> >* q;
};
void* seq_cons_test_worker(void* arg) {
    struct worker_arg* warg = (worker_arg*) arg;
    int id = warg->id;
    mpsc<pair<int, int> >* q = warg->q;
    free(warg);
    for (int i = 0; i < NITEMS; i++) {
        q->put(pair<int, int>(id, i));
    }
    return NULL;
}
void seq_cons_test() {
    mpsc<pair<int, int> > q;
    map<int, int> counts;
    for (int i = 0; i < NTHREADS; i++) {
        struct worker_arg* warg = (struct worker_arg*) Malloc(sizeof(worker_arg));
        warg->id = i;
        warg->q = &q;
        pthread_t thread;
        Pthread_create(&thread, NULL, seq_cons_test_worker, warg);
        Pthread_detach(thread);
        counts[i] = 0;
    }
    do {
        const pair<int, int>* got = q.get();
        if (got) {
            assert(counts[got->first] == got->second);
            if (++counts[got->first] == NITEMS) {
                counts.erase(got->first);
            }
        } else {
            sched_yield();
        }
    } while (counts.size());
}
int main() {
    basic_test();
    #define NTRIALS 8000
    for (size_t i = 0; i < NTRIALS; i++) {
        printf("%04X / %04X\b\b\b\b\b\b\b\b\b\b\b", i, NTRIALS);
        if (i < NTRIALS / 2) {
            para_test();
        } else {
            seq_cons_test();
        }
    }       
    printf("              \b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    // verify sequential consistency
    return 0;
}
