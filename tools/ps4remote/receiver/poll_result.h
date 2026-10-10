#pragma once
#include <cstdint>
struct ResultPoll {
    int transport=-1,operation=0;
    bool complete=false;
    unsigned polls=0;
};
// Deadline bounds retries, not time spent inside a native IPC call.
template<class Clock,class Read,class Wait>
ResultPoll poll_result(uint64_t deadline,Clock clock,Read read,Wait wait) {
    ResultPoll out;
    while(clock()<deadline) {
        out.transport=read(out.operation);++out.polls;
        if((uint32_t)out.transport!=0x80020023U &&
           !(out.transport==0 && (uint32_t)out.operation==0x80020023U)) {
            out.complete=true;return out;
        }
        wait();
    }
    return out;
}
