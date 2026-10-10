#include "../receiver/async_launch.h"
#include <cassert>
int main() {
 const int denied=(int)0x8094000fU;
 AsyncLaunchState s;assert(s.allowed(false)&&!s.allowed(true));
 s.begin(false);s.submitted(false,0,0); // ticket zero is valid
 assert(!s.allowed(false)&&!s.allowed(true));
 assert(!s.completed(-123));assert(s.ticket==0&&!s.allowed(true));
 assert(s.completed(denied));assert(s.ticket==-1&&s.allowed(true));
 assert(!s.allowed(false)); // never resubmit control
 s.begin(true);assert(!s.allowed(true));s.submitted(true,0,1);
 assert(!s.completed(denied)&&s.ticket==1&&!s.allowed(true));
 AsyncLaunchState immediate;immediate.begin(false);immediate.submitted(false,denied,-1);
 assert(immediate.allowed(true));
 AsyncLaunchState success;success.begin(false);success.submitted(false,0,2);
 assert(!success.completed(0x60000001)&&!success.allowed(true));
 AsyncLaunchState unknown;unknown.begin(false);unknown.submitted(false,-123,-1);
 assert(!unknown.allowed(true));
}
