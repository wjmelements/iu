#include "concat.h"

#include <assert.h>

#include "capitalC.h"

void basic_test() {
    concat<int> c;
    assert(c.empty());
    assert(!c.ready());
    c.put(1);
    assert(c.ready());
    assert(!c.empty());
    assert(!c.depleted());
    assert(!c.closed());

    assert(c.peek() == 1);
    assert(c.get() == 1);

    assert(c.empty());
    assert(!c.ready());

    concat<int>* d = new concat<int>;
    assert(d->empty());
    assert(!d->ready());

    c.give(d);
    c.put(2);
    assert(!c.ready());

    d->put(3);
    assert(c.ready());
    assert(d->ready());
    assert(!c.empty());

    assert(d->peek() == 3);
    assert(d->get() == 3);
    assert(!c.ready());

    d->put(4);
    c.skip();
    d->close();
    assert(d->closed());

    assert(c.peek() == 2);
    assert(c.get() == 2);
}

void give_no_consume_test() {
    concat<int> c;
    concat<int>* d = new concat<int>;
    c.give(d);
    assert(!c.empty());
    c.put(1);
    assert(!c.ready());
    assert(!d->closed());
    assert(!c.closed());
}

void give_consume_test() {
    concat<int> c;
    concat<int>* d = new concat<int>;
    c.give(d);
    c.put(1);
    d->put(2);
    assert(c.ready());
    assert(2 == c.get());
    assert(!c.ready());
    d->close();
    assert(c.ready());
    assert(!c.closed());
    assert(1 == c.get());
}

void* abc(void* arg) {
    concat<char>* prod = (concat<char>*) arg;
    for (char let = 'a'; let <= 'z'; let++) {
        prod->put(let);
    }
    prod->close();
    return NULL;
}

void abc_test() {
    concat<char> str;
    abc(&str);
    char curr = 'a';
    while (str.ready()) {
        assert(str.get() == curr++);
    }
    assert(str.empty());
    assert(str.depleted());
    assert(str.closed());
}

template<bool check_depleted> void abc_para_test() {
    concat<char> str
    ,   *dep1 = new concat<char>
    ,   *dep2 = new concat<char>
    ;
    assert(!str.ready());
    assert(!dep1->closed());
    assert(!dep2->closed());
    pthread_t thread[2];
    Pthread_create(&thread[0], NULL, abc, dep1);
    Pthread_detach(thread[0]);
    Pthread_create(&thread[1], NULL, abc, dep2);
    Pthread_detach(thread[1]);
    str.give(dep1);
    str.give(dep2);
    str.close();
    for (int i = 0; i < 2; i++) {
        for (char let = 'a'; let <= 'z'; let++) {
            if (check_depleted) {
                assert(!str.depleted());
                assert(str.ready());
            }
            assert(str.get() == let);
        }
    }
    assert(!str.ready());
    assert(str.depleted());
    assert(str.closed());
}

#ifndef REPS
#define REPS 10000
#endif
int main() {
    basic_test();
    give_no_consume_test();
    give_consume_test();
    abc_test();
    for (unsigned int i = 0; i < REPS; i++) {
        printf("%04X / %04X\b\b\b\b\b\b\b\b\b\b\b", i, REPS);
        abc_para_test<true>();
        abc_para_test<false>();
    }
    fputs("              \b\b\b\b\b\b\b\b\b\b\b\b\b\b", stdout);
    return 0;
}
