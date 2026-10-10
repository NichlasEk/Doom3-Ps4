// OpenOrbis libc++ references this ABI but the installed libc lacks it.
// This executable links its C++ libraries statically: no unloadable C++ DSOs.
#include <pthread.h>
#include <cstdlib>
#include <climits>

namespace {
struct Destructor {
    void (*call)(void*);
    void* object;
    Destructor* next;
};
pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;
int keyError = -1;

void drain(void* value)
{
    if (pthread_setspecific(key, value)) std::abort();
    for (;;) {
        auto* item = static_cast<Destructor*>(pthread_getspecific(key));
        if (!item) return;
        // Publish the remaining stack before calling user code. A callback can
        // register another destructor, which must run before older entries.
        if (pthread_setspecific(key, item->next)) std::abort();
        auto call = item->call;
        auto object = item->object;
        std::free(item);
        call(object);
    }
}
void drainMain()
{
    // Main-thread TLS cleanup is not necessarily performed by process exit.
    // Bounded like POSIX key destructor iterations; callbacks may register again.
    for (int i = 0; i < PTHREAD_DESTRUCTOR_ITERATIONS; ++i) {
        void* value = pthread_getspecific(key);
        if (!value) return;
        drain(value);
    }
}
void initialize()
{
    keyError = pthread_key_create(&key, drain);
    if (!keyError && std::atexit(drainMain)) {
        pthread_key_delete(key);
        keyError = -1;
    }
}
}
extern "C" int __cxa_thread_atexit_impl(void (*call)(void*), void* object, void*)
{
    if (pthread_once(&once, initialize) || keyError) return -1;
    auto* item = static_cast<Destructor*>(std::malloc(sizeof(Destructor)));
    if (!item) return -1;
    *item = {call, object, static_cast<Destructor*>(pthread_getspecific(key))};
    if (pthread_setspecific(key, item)) {
        std::free(item);
        return -1;
    }
    return 0;
}
