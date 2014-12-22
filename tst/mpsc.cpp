#include "mpsc.h"

#include <assert.h>

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

int main() {
    basic_test();
    return 0;
}
