#include "../receiver/poll_result.h"
#include <cassert>
int main() {
 uint64_t now=0;int calls=0;
 auto r=poll_result(3,[&]{return now;},[&](int &op){++calls;op=0;return (int)0x80020023U;},[&]{++now;});
 assert(!r.complete&&r.polls==3&&calls==3);
 now=0;calls=0;
 r=poll_result(3,[&]{return now;},[&](int &op){op=++calls==1?(int)0x80020023U:(int)0x80940031U;return 0;},[&]{++now;});
 assert(r.complete&&r.polls==2&&(uint32_t)r.operation==0x80940031U);
 now=0;r=poll_result(3,[&]{return now;},[](int &){return -5;},[&]{++now;});
 assert(r.complete&&r.polls==1&&r.transport==-5);
 now=3;r=poll_result(3,[&]{return now;},[](int &){assert(false);return 0;},[]{});
 assert(!r.complete&&r.polls==0);
}
