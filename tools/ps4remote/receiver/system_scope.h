#pragma once
#include "credentials.h"
#include <unistd.h>

// Process-wide credentials: dispatch is serial. Only bounded native operations
// (including bind) belong here, never HTTP waits or app launch/child creation.
// A crash cannot unwind this scope.
class SystemScope {
    CredentialObservation before{};
    bool attempted=false, ready=false;
    static CredentialObservation read() {
        CredentialObservation o{};o.expected_pid=getpid();o.expected_uid=getuid();
        receiver_read_credentials(&o);return o;
    }
    static bool matches(const CredentialObservation &a,const CredentialObservation &b) {
        return a.complete==0x524d544352454431ULL && a.pid==b.pid && a.uid==b.uid &&
            a.auth==b.auth && a.attributes==b.attributes && a.capabilities==b.capabilities;
    }
public:
    explicit SystemScope(bool supported) {
        if(!supported){log("BGFT system scope unsupported firmware",-1);return;}
        before=read();
        if(before.complete!=0x524d544352454431ULL || before.auth!=0x3800000000000010ULL ||
           before.attributes!=0x2000038000000000ULL || before.capabilities!=0xff00) {
            log("BGFT system scope baseline mismatch",-1);return;
        }
        attempted=true;
        CapabilityChange change{};change.expected=before;change.replacement=before.attributes|(1ULL<<62);
        receiver_change_system(&change);
        auto expected=before;expected.attributes=change.replacement;
        ready=change.applied==0x524d544341505331ULL && matches(read(),expected);
        log("BGFT system scope enabled",ready?0:-1);
    }
    ~SystemScope() {
        if(!attempted)return;
        auto current=read();
        if(current.complete==0x524d544352454431ULL && current.attributes==(before.attributes|(1ULL<<62))) {
            CapabilityChange restore{};restore.expected=current;restore.replacement=before.attributes;
            receiver_change_system(&restore);
        }
        bool restored=matches(read(),before);
        log("BGFT system scope restored",restored?0:-1);
        if(!restored)_exit(5);
    }
    explicit operator bool() const {return ready;}
    SystemScope(const SystemScope&)=delete;
    SystemScope& operator=(const SystemScope&)=delete;
};
