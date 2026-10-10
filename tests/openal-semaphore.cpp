// Exercise the PS4-specific OpenAL semaphore with real host pthreads.
#include "threads.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
int main() {
    al::semaphore ready(2);
    assert(ready.try_wait()); assert(ready.try_wait()); assert(!ready.try_wait());
    std::atomic<int> done{0};
    std::vector<std::thread> waiters;
    for (int n=0;n<4;++n) waiters.emplace_back([&] {
        for(int i=0;i<1000;++i) ready.wait();
        ++done;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    assert(done==0); // Empty semaphore must block.
    for(int i=0;i<4000;++i) ready.post();
    for(auto& t:waiters) t.join();
    assert(done==4); assert(!ready.try_wait());
}
