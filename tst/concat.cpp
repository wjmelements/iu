#include "concat.h"

#include <assert.h>

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
    assert(!c.ready());
    assert(!d->closed());
}

int main() {
    basic_test();
    give_no_consume_test();
    return 0;
}
