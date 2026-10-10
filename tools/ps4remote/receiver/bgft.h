// Local ABI: flatz/ps4_stub_lib_maker_v2/include/bgft.h (64-bit Orbis).
#pragma once
#include <cstddef>
#include <cstdint>
struct BgftInit { void *heap; size_t size; };
struct BgftParam {
 int32_t user, entitlement;
 const char *id,*url,*exUrl,*name,*icon,*sku;
 int32_t option;
 const char *scenario,*release,*type,*subtype;
 uint64_t size;
};
struct BgftStorage { BgftParam params; uint32_t slot; };
struct BgftProgress {
 uint32_t bits; int32_t error;
 uint64_t length,transferred,total,totalTransferred;
 uint32_t index,count,seconds,totalSeconds;
 int32_t preparing,copying;
};
static_assert(sizeof(BgftParam)==104 && offsetof(BgftParam,size)==96,"BGFT param ABI");
static_assert(sizeof(BgftStorage)==112 && sizeof(BgftProgress)==64,"BGFT ABI");
