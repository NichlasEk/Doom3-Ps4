#pragma once
#include <stdint.h>
// Inclusive intervals: decode includes file_read, upload includes CPU packing and most submit/wait work.
// Thread sums may exceed wall time. No GPU timestamp claims.
enum D3LoadStage { D3LP_FILE, D3LP_DECODE, D3LP_HASH, D3LP_MIPS,
                   D3LP_UPLOAD, D3LP_WAIT, D3LP_PACK, D3LP_COUNT };
bool D3LoadActive();
void D3LoadBegin();
void D3LoadEnd();
uint64_t D3LoadStart();
void D3LoadFinish(D3LoadStage stage, uint64_t start);
struct D3LoadTimer {
    D3LoadStage stage; uint64_t start;
    explicit D3LoadTimer(D3LoadStage s):stage(s),start(D3LoadStart()) {}
    ~D3LoadTimer() { D3LoadFinish(stage,start); }
    D3LoadTimer(const D3LoadTimer&)=delete;
    D3LoadTimer& operator=(const D3LoadTimer&)=delete;
};
struct D3LoadWindow {
    D3LoadWindow() { D3LoadBegin(); }
    ~D3LoadWindow() { D3LoadEnd(); }
};
