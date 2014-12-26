#include <assert.h>

#ifndef REPS
#define REPS 1000
#endif

#include "stream.h"
// thread to read and verify that the stream has a to z and nothing more
// may randomly cancel at any point
// used in testing for verifying correctness of stream content
void* reader(void* arg) {
    key<char>* args = (key<char>*) arg;
    for (char expected = 'a'; expected <= 'z'; expected++) {
        if ((random() & 0x1F) < 1) {
            // cancel with probability ~1/32
            cancel(args);
            assert(canceled(args));
            return NULL;
        }
        if ((random() & 0x1F) < 9) {
            // skip with probability ~75/256
            skip(args);
        } else {
            if ((random() & 0x1F) < 9 ) {
                // peek with probability ~41/200
                char peeked = peek(args);
                assert(expected == peeked);
                expected--;
            } else {
                // from with probability ~17/32
                char actual = from(args);
                assert(expected == actual);
            }
        }
    }
    if (random() / 31 > RAND_MAX >> 5) {
        // cancel with probability ~1/32
        cancel(args);
        assert(canceled(args));
    } else {
        assert(depleted(args));
        assert(closed(args));
    }
    return EXIT_SUCCESS;
}
// thread to populate a stream with the charaters g to s and then close
// used in testing to populate a stream to be given
void* populate_g2s(void* arg) {
    stream<char>* bet = (stream<char>*) arg;
    for (char let = 'g'; let <= 's'; let++) {
        bet->put(let);
    }
    bet->close();
    return EXIT_SUCCESS;
}
// test no ports behavior
void test_nolisten() {
    stream<char>* out = new stream<char>(0);
    key<char>* ret = out->listen();
    out->put('a');
    assert(ret == NULL);
    delete out;
}
// tests listening more than the number of ports
void test_overlisten() {
    stream<char>* out = new stream<char>(1);
    char out_expected = 'a';
    key<char>* out_key = out->listen();
    out->put(out_expected);
    key<char>* ret = out->listen();
    assert(ret == NULL);
    char out_actual = from(out_key);
    assert(out_actual == out_expected);
    delete out;
    free(out_key);
}
// tests basic skip and peek functionality
void test_skip() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    char to_skip1 = 'a';
    char to_peek = 'b';
    char to_skip2 = 'c';
    out->put(to_skip1);
    skip(out_key);
    out->put(to_peek);
    char peeked = peek(out_key);
    assert(peeked == to_peek);
    char taken = from(out_key);
    assert(taken == to_peek);
    out->put(to_skip2);
    out->close();
    skip(out_key);
    assert(depleted(out_key));
    delete out;
    free(out_key);
}
// tests the ability to retrieve data as soon as it is available
void test_weave() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    for (char let = 'a'; let <= 'z'; let++) {
        assert(!ready(out_key));
        out->put(let);
        assert(ready(out_key));
        char actual = from(out_key);
        assert(let == actual);
    }
    assert(empty(out_key));
    assert(!ready(out_key));
    out->close();
    assert(closed(out_key));
    delete out;
    free(out_key);
}
// tests the scenario where all things were in the stream before any listening
void test_serial() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    for (char let = 'a'; let <= 'z'; let++) {
        out->put(let);
    }
    out->close();
    for (char let = 'a'; let <= 'z'; let++) {
        char actual = from(out_key);
        assert(let == actual);
    }
    assert(closed(out_key));
    assert(depleted(out_key));
    delete out;
    free(out_key);
}
// tests cancel behaviors when canceled before anything is put
void test_cancelbefore() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    cancel(out_key);
    for (char let = 'a'; let <= 'z'; let++) {
        out->put(let);
    }
    out->close();
    assert(canceled(out_key));
    delete out;
    free(out_key);
}
// tests cancel behaviors when canceled after everything is put and get
void test_cancelafter() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    for (char let = 'a'; let <= 'z'; let++) {
        out->put(let);
    }
    out->close();
    cancel(out_key);
    assert(canceled(out_key));
    delete out;
    free(out_key);
}
void test_giveempties() {
    size_t reps = REPS / 4 < 1 ? 1 : REPS / 2;
    for (size_t i = 0; i < reps; i++) {
        stream<char>* out = new stream<char>(1);
        key<char>* out_key = out->listen();
        stream<char>* one = new stream<char>(1);
        key<char>* one_key = one->listen();
        stream<char>* two = new stream<char>(1);
        key<char>* two_key = two->listen();
        stream<char>* three = new stream<char>(1);
        key<char>* three_key = three->listen();
        out->give(one_key);
        out->give(two_key);
        out->give(three_key);
        out->close();
        three->close();
        two->close();
        one->close();
        assert(depleted(out_key));
        assert(depleted(one_key));
        assert(depleted(two_key));
        assert(depleted(three_key));
        delete out;
        free(out_key);
        delete one;
        free(one_key);
        delete two;
        free(two_key);
        delete three;
        free(three_key);
    }
}
// tests give by populating both lists first, then giving one to the other
void test_giveafter() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    stream<char>* to_give = new stream<char>(1);
    key<char>* key_to_give = to_give->listen();
    assert(empty(out_key));
    for (char let = 'a'; let <= 'z'; let++) {
        out->put(let);
        to_give->put(let);
    }
    out->give(key_to_give);
    for (int i = 0; i < 2; i++) {
        for (char let = 'a'; let <= 'z'; let++) {
            char actual = from(out_key);
            assert(let == actual);
        }
    }
    assert(!empty(out_key));
    out->close();
    to_give->close();
    assert(empty(out_key));
    assert(closed(out_key));
    assert(depleted(out_key));
    assert(empty(out_key));
    delete out;
    free(out_key);
    delete to_give;
    free(key_to_give);
}
// tests give where we populate both streams, get all from the first, give, then get the rest
void test_givedelay() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    stream<char>* to_give = new stream<char>(1);
    key<char>* key_to_give = to_give->listen();
    for (char let = 'a'; let <= 'z'; let++) {
        out->put(let);
        to_give->put(let);
    }
    for (char let = 'a'; let <= 'z'; let++) {
        char actual = from(out_key);
        assert(let == actual);
    }
    out->give(key_to_give);
    for (char let = 'a'; let <= 'z'; let++) {
        char actual = from(out_key);
        assert(let == actual);
    }
    to_give->close();
    out->close();
    assert(depleted(out_key));
    assert(closed(out_key));
    delete out;
    free(out_key);
    delete to_give;
    free(key_to_give);
}
// tests give behavior where we give before we put
void test_givebefore() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    stream<char>* to_give = new stream<char>(1);
    key<char>* key_to_give = to_give->listen();
    out->give(key_to_give);
    for (char let = 'a'; let <= 'z'; let++) {
        to_give->put(let);
        char expected = from(out_key);
        assert(let == expected);
        out->put(let);
    }
    to_give->close();
    for (char let = 'a'; let <= 'z'; let++) {
        char expected = from(out_key);
        assert(let == expected);
    }
    out->close();
    assert(closed(out_key));
    assert(depleted(out_key));
    delete out;
    free(out_key);
    delete to_give;
    free(key_to_give);
}
// verify that giving a depleted stream works as indented
void test_givedepleted() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    stream<char>* to_give = new stream<char>(1);
    key<char>* key_to_give = to_give->listen();
    for (char let = 'a'; let <= 'z'; let++) {
        to_give->put(let);
        char expected = from(key_to_give);
        assert(let == expected);
        out->put(let);
    }
    to_give->close();
    assert(depleted(key_to_give));
    out->give(key_to_give);
    for (char let = 'a'; let <= 'z'; let++) {
        char expected = from(out_key);
        assert(let == expected);
    }
    out->close();
    assert(depleted(out_key));
    assert(closed(out_key));
    delete out;
    free(out_key);
    delete to_give;
    free(key_to_give);
};
// test skip/peek when give
void test_giveskip() {
    stream<char>* out = new stream<char>(1);
    key<char>* out_key = out->listen();
    stream<char>* to_give = new stream<char>(1);
    key<char>* to_give_key = to_give->listen();
    char to_putbefore = 'a';
    char from_given1 = 'b';
    char from_given2 = 'c';
    char to_putafter = 'd';
    out->put(to_putbefore);
    char first = peek(out_key);
    assert(first == to_putbefore);
    skip(out_key);
    out->give(to_give_key);
    to_give->put(from_given1);
    char second = peek(out_key);
    assert(second == from_given1);
    skip(out_key);
    to_give->put(from_given2);
    assert(!depleted(out_key));
    char third = peek(out_key);
    assert(third == from_given2);
    skip(out_key);
    to_give->close();
    assert(depleted(to_give_key));
    out->put(to_putafter);
    char fourth = peek(out_key);
    assert(fourth == to_putafter);
    skip(out_key);
    out->close();
    assert(depleted(out_key));
    delete out;
    free(out_key);
    delete to_give;
    free(to_give_key);
}
// test parallel functionality
// REPS should be 1 if running through helgrind or memcheck
void test_parallel() {
    for (unsigned int i = 0; i < REPS; i++) {
        printf("%04X / %04X\b\b\b\b\b\b\b\b\b\b\b", i, REPS);
        pthread_t thread1;
        pthread_t thread2;
        stream<char>* out = new stream<char>(2);
        key<char>* out_key1 = out->listen();
        key<char>* out_key2 = out->listen();
        pthread_create(&thread1, NULL, reader, out_key1);
        pthread_create(&thread2, NULL, reader, out_key2);
        for (char let = 'a'; let <= 'z'; let++) {
            out->put(let);
        }
        out->close();
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        delete out;
        free(out_key1);
        free(out_key2);
    }
    printf("              \b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}
void test_giveparallel() {
    // test parallel give functionality
    for (unsigned int i = 0; i < REPS; i++) {
        printf("%04X / %04X\b\b\b\b\b\b\b\b\b\b\b", i, REPS);
        pthread_t thread1;
        pthread_t thread2;
        pthread_t thread3;
        stream<char>* out2 = new stream<char>(2);
        key<char>* out2_key1 = out2->listen();
        key<char>* out2_key2 = out2->listen();
        stream<char>* out3 = new stream<char>(1);
        key<char>* out3_key = out3->listen();
        pthread_create(&thread1, NULL, reader, out2_key1);
        pthread_create(&thread2, NULL, reader, out2_key2);
        pthread_create(&thread3, NULL, populate_g2s, out3);
        for (char let = 'a'; let <= 'f'; let++) {
            out2->put(let);
        }
        out2->give(out3_key);
        for (char let = 't'; let <= 'z'; let++) {
            out2->put(let);
        }
        out2->close();
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        pthread_join(thread3, NULL);
        delete out2;
        delete out3;
        free(out2_key1);
        free(out2_key2);
        free(out3_key);
    }
    printf("              \b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}
int main() {
    test_nolisten();
    test_overlisten();
    test_skip();
    test_weave();
    test_serial();
    test_cancelbefore();
    test_cancelafter();
    test_giveempties();
    test_giveafter();
    test_givedelay();
    test_givebefore();
    test_givedepleted();
    test_giveskip();
    test_parallel();
    test_giveparallel();
    return 0;
}
