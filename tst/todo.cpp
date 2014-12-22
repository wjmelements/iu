#include "todo.h"

#include <assert.h>

size_t count = 0;
void func(void* arg) {
    count++;
}
void basic_test() {
    add_todo(func, NULL);
    add_todo(func, NULL);
    add_todo(func, NULL);
    assert(count == 0);
    do_todos();
    assert(count == 3);
}
int main() {
    basic_test();
    return 0;
}
