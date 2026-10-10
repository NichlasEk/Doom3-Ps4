#include "../receiver/lnc960.h"
#include <cassert>
#include <cstring>
int main() {
    LncLaunch960 launch{};
    launch.user=0x12345678;
    const uint32_t expected[]={24,0x12345678,0,0,0,0};
    assert(sizeof(expected)==sizeof(launch));
    assert(!memcmp(&launch,expected,sizeof(launch)));
    LocalProcess960 params{};
    const uint32_t defaults[]={40,0xffffffff,1,0xffffffff,0,0,0,0,0,0};
    assert(sizeof(defaults)==sizeof(params));
    assert(!memcmp(&params,defaults,sizeof(params)));
}
