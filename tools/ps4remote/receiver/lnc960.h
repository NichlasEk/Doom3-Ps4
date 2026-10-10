#pragma once
#include <cstddef>
#include <cstdint>
// Fixed-size wire buffers observed in this console's 9.60 LncUtil marshalling.
// Unknown words stay zero/default; do not use SDK's 32-byte LncAppParam here.
struct LncLaunch960 {
    uint32_t size=24;
    int32_t user=-1;
    uint32_t words[4]={};
};
static_assert(sizeof(LncLaunch960)==24 && offsetof(LncLaunch960,user)==4,"9.60 launch wire");
// Public AddLocalProcess wrapper maps these 40 bytes into its native buffer.
// These defaults reproduce the wrapper's own template, with non-null params.
struct LocalProcess960 {
    uint32_t size=40;
    int32_t unknown4=-1;
    uint32_t mode=1;
    int32_t unknown12=-1;
    uint32_t unknown16=0,padding=0;
    uint64_t unknown24=0;
    uint32_t unknown32=0,tail=0;
};
static_assert(sizeof(LocalProcess960)==40 && offsetof(LocalProcess960,unknown24)==24,"9.60 local process wrapper");
