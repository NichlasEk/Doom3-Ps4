#include <cassert>
#include <stdexcept>
#include <sys/wait.h>
#include "../receiver/credentials.h"
static void log(const char*,int) {}
#include "../receiver/system_scope.h"
static CredentialObservation state;
static int fault, writes;
extern "C" long receiver_read_credentials(CredentialObservation *o) {
    *o=state;return 0;
}
extern "C" long receiver_change_system(CapabilityChange *c) {
    ++writes;
    if(fault==1)return -1; // Rejected write.
    if(fault==3 && writes==2)return -1; // Restore failure.
    state.attributes=c->replacement;
    if(fault!=2)c->applied=0x524d544341505331ULL; // Lost acknowledgment.
    return 0;
}
static void reset(int f=0) {
    state={};state.complete=0x524d544352454431ULL;state.pid=getpid();state.uid=getuid();
    state.auth=0x3800000000000010ULL;state.attributes=0x2000038000000000ULL;
    state.capabilities=0xff00;fault=f;writes=0;
}
int main() {
    reset();{SystemScope s(false);assert(!s);}assert(writes==0);
    reset();state.auth=0;{SystemScope s(true);assert(!s);}assert(writes==0);
    reset();{SystemScope s(true);assert(s);assert(state.attributes==0x6000038000000000ULL);}
    assert(state.attributes==0x2000038000000000ULL && writes==2);
    reset();try {SystemScope s(true);assert(s);throw std::runtime_error("service failure");}catch(...){}
    assert(state.attributes==0x2000038000000000ULL && writes==2);
    reset(1);{SystemScope s(true);assert(!s);}assert(state.attributes==0x2000038000000000ULL);
    reset(2);{SystemScope s(true);assert(!s);}assert(state.attributes==0x2000038000000000ULL && writes==2);
    pid_t child=fork();assert(child>=0);
    if(child==0){reset(3);{SystemScope s(true);assert(s);}_exit(99);}
    int status=0;assert(waitpid(child,&status,0)==child);assert(WIFEXITED(status)&&WEXITSTATUS(status)==5);
}
