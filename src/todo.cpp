#include "todo.h"
struct todo {
    func_t func;
    void* arg;
};
mpsc<todo> todos;
void do_todos() {
    while (1) {
        const struct todo* job = todos.get();
        if (job) {
            job->func(job->arg);
        } else {
            break;
        }
    }
}
void add_todo(func_t func, void* arg) {
    struct todo put;
    put.func = func;
    put.arg = arg;
    todos.put(put);
}
