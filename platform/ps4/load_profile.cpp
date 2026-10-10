#include "load_profile.h"
#include <atomic>
#include <time.h>
#include "sys/platform.h"
#include "sys/sys_public.h"
static std::atomic<bool> active(false);
static std::atomic<uint64_t> elapsed[D3LP_COUNT], calls[D3LP_COUNT];
static uint64_t windowStart;
static uint64_t nowNs() {
    timespec t={};
    if(clock_gettime((clockid_t)4,&t)!=0) return 0;
    return uint64_t(t.tv_sec)*1000000000ULL+uint64_t(t.tv_nsec);
}
bool D3LoadActive() { return active.load(std::memory_order_acquire); }
void D3LoadBegin() {
    for(int i=0;i<D3LP_COUNT;++i) { elapsed[i]=0;calls[i]=0; }
    windowStart=nowNs();active.store(true,std::memory_order_release);
}
uint64_t D3LoadStart() {
    return active.load(std::memory_order_acquire)?nowNs():0;
}
void D3LoadFinish(D3LoadStage stage,uint64_t start) {
    if(!start) return;
    const uint64_t end=nowNs();
    if(end<start) return;
    elapsed[stage].fetch_add(end-start,std::memory_order_relaxed);
    calls[stage].fetch_add(1,std::memory_order_relaxed);
}
void D3LoadEnd() {
    active.store(false,std::memory_order_release);
    const uint64_t end=nowNs();
    Sys_Printf("PS4 LOADPROFILE wall_ms=%.3f inclusive=1 thread_sums=1\n",
               end>=windowStart?(end-windowStart)/1000000.0:0.0);
    const char *names[]={"file_read_inflate","image_program","image_hash","mip_chain","texture_upload","submit_wait","cpu_pack"};
    for(int i=0;i<D3LP_COUNT;++i)
        Sys_Printf("PS4 LOADPROFILE stage=%s calls=%llu sum_ms=%.3f\n",names[i],
                   (unsigned long long)calls[i].load(),elapsed[i].load()/1000000.0);
}
