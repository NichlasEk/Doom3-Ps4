#pragma once
#include <cstdint>
// 9.60 control completion NOT_ALLOWED was corroborated by ShellCore's
// isSystemProcess rejection. Other errors retain the outstanding ticket.
struct AsyncLaunchState {
    int ticket=-1,controlResult=-1;
    bool controlAttempted=false,scopedAttempted=false;
    bool allowed(bool scoped) const {
        return ticket<0 && !(scoped?scopedAttempted:controlAttempted) &&
            (!scoped || (controlAttempted && (uint32_t)controlResult==0x8094000fU));
    }
    void begin(bool scoped) { if(scoped)scopedAttempted=true;else controlAttempted=true; }
    void submitted(bool scoped,int result,int issuedTicket) {
        if(!scoped)controlResult=result;
        if(result==0 && issuedTicket>=0)ticket=issuedTicket;
    }
    bool completed(int result) {
        if(ticket>=0 && controlAttempted && !scopedAttempted && (uint32_t)result==0x8094000fU) {
            controlResult=result;ticket=-1;return true;
        }
        return false;
    }
};
