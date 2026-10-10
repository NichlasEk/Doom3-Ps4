// PS4 Remote Receiver 0.24: foreground diagnostic/install/launch bridge.
#include <cstdio>
#include "bgft.h"
#include "lnc960.h"
#include "async_launch.h"
#include "poll_result.h"
#include "credentials.h"
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <netinet/in.h>
#include <string>
#include <cctype>
#include <sys/stat.h>
#include <sys/time.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>

static void (*ipcConfig)(void*);
static size_t (*ipcEstimate)(void*);
static int (*ipcCreate)(void**,const void*,void*,void*);
static int (*ipcConnect)(void*,const void*,size_t,int*);
static int (*ipcDisconnect)(void*);
static int (*ipcDestroy)(void*);
static bool ipcTested=false;
static bool ipcCompatible=false;
static int (*bgInit)(BgftInit*);
static int (*bgPublicInit)(BgftInit*);
static int bgPublicResult=-1;
static BgftInit bgPublicHeap={};
static int (*bgRegister)(BgftStorage*,int*);
static int (*bgStart)(int);
static int (*bgProgress)(int,BgftProgress*);
static int bgResult=-1;
static BgftInit bgHeap={};
static int ownedTask=-1;
static int (*appInit)();
static int (*appExists)(const char*,int*);
static int (*pkgTitle)(const char*,char*,int*);
static int (*pkgInstall)(const char*,void*);
static int appResult = -1;
static int (*launchApp)(const char*,const char**,LncAppParam*);
static int (*lncInit)();
static int (*directLaunch)(const char*,const char**,LncLaunch960*);
static int (*startLaunch)(const char*,const char**,LncLaunch960*,int*);
static int (*launchAsyncResult)(int);
static unsigned char *lncState=nullptr;
static AsyncLaunchState asyncLaunch;
static int (*addLocal)(int,const char*,const char**,LocalProcess960*);
static int (*registerDaemon)();
static int workerResult=-1,daemonResult=-1;
static bool workerAttempted=false;
#ifdef REMOTE_WORKER
static constexpr bool isWorker=true;
static constexpr int serverPort=12802;
static const char *role="worker";
static const char *logPath="/data/ps4remote/worker.log";
#else
static constexpr bool isWorker=false;
static constexpr int serverPort=12801;
static const char *role="receiver";
static const char *logPath="/data/ps4remote/receiver.log";
#endif
static int lncResult=-1;
static int bigAppId=-1,miniAppId=-1;
static unsigned launchTestAttempts=0;
static constexpr unsigned launchTestLimit=10;
static char key[65];
static FILE *logfile;
static void log(const char *event,int result) {
    if (logfile) { fprintf(logfile,"%s result=0x%08x\n",event,(unsigned)result);fflush(logfile); }
}
#include "system_scope.h"
template<class Call> static int bgft_call(const char *event,Call call) {
    OrbisKernelSwVersion fw={};fw.Size=sizeof(fw);
    bool supported=sceKernelGetSystemSwVersion(&fw)==0 && (fw.Version&0xffff0000U)==0x09600000U;
    int result=-1;
    log(event,0);
    {
        SystemScope scope(supported);
        if(scope)result=call();
        log("BGFT scoped call returned",result);
    } // Restore verified before returning to HTTP handling.
    return result;
}
static bool title_valid(const std::string &s) {
    if(s.size()!=9 || s.substr(0,4)=="NPXS" || s=="RMTT00001")return false;
    for(int i=0;i<9;++i)if(i<4 ? s[i]<'A'||s[i]>'Z' : s[i]<'0'||s[i]>'9')return false;
    return true;
}
static bool hash_valid(const std::string &s) {
    if(s.size()!=64)return false;
    for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
static bool send_all(int s,const std::string &text) {
    size_t n=0;while(n<text.size()){int r=sceNetSend(s,text.data()+n,text.size()-n,0);if(r<=0)return false;n+=r;}return true;
}
static void response(int s,int code,const std::string &body) {
    send_all(s,"HTTP/1.1 "+std::to_string(code)+" Result\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body);
}
static int receive(int client,void *buffer,size_t size,uint64_t deadline) {
    while(sceKernelGetProcessTime()<deadline) {
        // SceNet uses BSD MSG_DONTWAIT (0x80), not musl's Linux value.
        int n=sceNetRecv(client,buffer,size,0x80);
        if(n>=0)return n;
        sceKernelUsleep(5000);
    }
    return -1;
}
static void probe(const std::string &mode) {
    log(("probe begin "+mode).c_str(),0);
    if(mode=="app-slots") {
        bigAppId=sceSystemServiceGetAppIdOfBigApp();
        miniAppId=sceSystemServiceGetAppIdOfMiniApp();
        log("app slots big",bigAppId);log("app slots mini",miniAppId);
        return;
    }
    if(mode=="caps-test") {
        static bool attempted=false;
        OrbisKernelSwVersion fw={};fw.Size=sizeof(fw);
        if(sceKernelGetSystemSwVersion(&fw)!=0 || (fw.Version&0xffff0000U)!=0x09600000U || attempted){log("caps test blocked",-1);return;}
        attempted=true;
        CredentialObservation before={};before.expected_pid=getpid();before.expected_uid=getuid();
        receiver_read_credentials(&before);
        if(before.complete!=0x524d544352454431ULL || before.capabilities!=0xff00 || before.auth!=0x3800000000000010ULL || before.attributes!=0x2000038000000000ULL){log("caps baseline mismatch",-1);return;}
        log("caps control A begin",0);ipcTested=false;probe("ipc-connect");
        if(!ipcCompatible){log("caps IPC preflight failed",-1);return;}
        CapabilityChange change={};change.expected=before;change.replacement=UINT64_MAX;
        log("caps temporary change begin",0);receiver_change_capabilities(&change);
        CredentialObservation elevated={};elevated.expected_pid=getpid();elevated.expected_uid=getuid();receiver_read_credentials(&elevated);
        bool changed=change.applied==0x524d544341505331ULL && elevated.complete==0x524d544352454431ULL && elevated.capabilities==UINT64_MAX;
        log("caps temporary readback",changed?0:-1);
        if(changed){log("caps experiment B begin",0);ipcTested=false;probe("ipc-connect");}
        // Always inspect and restore after attempting a write, even if acknowledgment failed.
        CredentialObservation current={};current.expected_pid=getpid();current.expected_uid=getuid();receiver_read_credentials(&current);
        if(current.complete==0x524d544352454431ULL && current.capabilities==UINT64_MAX){
            CapabilityChange restore={};restore.expected=current;restore.replacement=before.capabilities;
            log("caps restore begin",0);receiver_change_capabilities(&restore);
        }
        CredentialObservation after={};after.expected_pid=getpid();after.expected_uid=getuid();receiver_read_credentials(&after);
        bool restored=after.complete==0x524d544352454431ULL && after.capabilities==before.capabilities && after.auth==before.auth && after.attributes==before.attributes && after.uid==before.uid;
        log("caps restoration verified",restored?0:-1);
        if(!restored){log("caps restoration unverified; exiting receiver",-1);_exit(5);}
        log("caps control A2 begin",0);ipcTested=false;probe("ipc-connect");
        log("caps test complete",0);return;
    }
    if(mode=="system-test") {
        static bool attempted=false;
        OrbisKernelSwVersion fw={};fw.Size=sizeof(fw);
        if(sceKernelGetSystemSwVersion(&fw)!=0 || (fw.Version&0xffff0000U)!=0x09600000U || attempted){log("system test blocked",-1);return;}
        attempted=true;
        CredentialObservation before={};before.expected_pid=getpid();before.expected_uid=getuid();
        receiver_read_credentials(&before);
        if(before.complete!=0x524d544352454431ULL || before.capabilities!=0xff00 || before.auth!=0x3800000000000010ULL || before.attributes!=0x2000038000000000ULL){log("system baseline mismatch",-1);return;}
        log("system control A begin",0);ipcTested=false;probe("ipc-connect");
        if(!ipcCompatible){log("system IPC preflight failed",-1);return;}
        CapabilityChange change={};change.expected=before;change.replacement=before.attributes | (1ULL<<62);
        log("system temporary change begin",0);receiver_change_system(&change);
        CredentialObservation elevated={};elevated.expected_pid=getpid();elevated.expected_uid=getuid();receiver_read_credentials(&elevated);
        bool changed=change.applied==0x524d544341505331ULL && elevated.complete==0x524d544352454431ULL && elevated.attributes==(before.attributes | (1ULL<<62));
        log("system temporary readback",changed?0:-1);
        if(changed){log("system experiment B begin",0);ipcTested=false;probe("ipc-connect");}
        // Always inspect and restore after attempting a write, even if acknowledgment failed.
        CredentialObservation current={};current.expected_pid=getpid();current.expected_uid=getuid();receiver_read_credentials(&current);
        if(current.complete==0x524d544352454431ULL && current.attributes==(before.attributes | (1ULL<<62))){
            CapabilityChange restore={};restore.expected=current;restore.replacement=before.attributes;
            log("system restore begin",0);receiver_change_system(&restore);
        }
        CredentialObservation after={};after.expected_pid=getpid();after.expected_uid=getuid();receiver_read_credentials(&after);
        bool restored=after.complete==0x524d544352454431ULL && after.capabilities==before.capabilities && after.auth==before.auth && after.attributes==before.attributes && after.uid==before.uid;
        log("system restoration verified",restored?0:-1);
        if(!restored){log("system restoration unverified; exiting receiver",-1);_exit(5);}
        log("system control A2 begin",0);ipcTested=false;probe("ipc-connect");
        log("system test complete",0);return;
    }
    if(mode=="credentials") {
        OrbisKernelSwVersion fw={};fw.Size=sizeof(fw);
        int r=sceKernelGetSystemSwVersion(&fw);log("credential firmware query",r);log("credential firmware",fw.Version);
        if(r!=0 || (fw.Version&0xffff0000U)!=0x09600000U){log("credentials unsupported firmware",-1);return;}
        static bool attempted=false;
        if(attempted){log("credentials already attempted",-1);return;}attempted=true;
        CredentialObservation o={};o.expected_pid=getpid();o.expected_uid=getuid();
        log("credentials read begin",0);long raw=receiver_read_credentials(&o);log("credentials syscall result",(int)raw);
        log("credentials callback entered",o.entered==0x524d544352454431ULL?0:-1);
        if(o.complete!=0x524d544352454431ULL){log("credentials observation incomplete",-1);return;}
        if(logfile){fprintf(logfile,"credentials pid=%u uid=%u auth=0x%016llx attributes=0x%016llx capabilities=0x%016llx\n",o.pid,o.uid,(unsigned long long)o.auth,(unsigned long long)o.attributes,(unsigned long long)o.capabilities);fflush(logfile);}
        log("credentials read complete",0);return;
    }
    if(mode=="internal")log("AppInst internal",(int)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_APP_INST_UTIL));
    else if(mode=="direct")log("AppInst direct",(int)sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx",0,nullptr,0,nullptr,nullptr));
    else if(mode=="sandbox") {
        const char *word=sceKernelGetFsSandboxRandomWord();
        if(word){char path[512];snprintf(path,sizeof(path),"/%s/common/lib/libSceAppInstUtil.sprx",word);
            log("AppInst sandbox",(int)sceKernelLoadStartModule(path,0,nullptr,0,nullptr,nullptr));}
        else log("sandbox word unavailable",-1);
    } else if(mode=="bgft")log("BGFT internal",(int)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_BGFT));
    struct Dependency {const char *mode;OrbisSysModuleInternal id;};
    const Dependency deps[]={
        {"dep-system",ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE},
        {"dep-user",ORBIS_SYSMODULE_INTERNAL_USER_SERVICE},
        {"dep-netctl",ORBIS_SYSMODULE_INTERNAL_NETCTL},
        {"dep-http",ORBIS_SYSMODULE_INTERNAL_HTTP},
        {"dep-ssl",ORBIS_SYSMODULE_INTERNAL_SSL},
        {"dep-np",ORBIS_SYSMODULE_INTERNAL_NP_COMMON}};
    for(const auto &dep:deps)if(mode==dep.mode) {
        log(dep.mode,0);log("dependency load returned",(int)sceSysmoduleLoadModuleInternal(dep.id));
    }
    OrbisKernelModule modules[256]={};size_t count=0;
    int listed=sceKernelGetModuleList(modules,256,&count);log("module list",listed);
    if(listed==0) for(size_t i=0;i<count && i<256;++i) {
        OrbisKernelModuleInfo info={};info.size=sizeof(info);
        if(!sceKernelGetModuleInfo(modules[i],&info))log(info.name,(int)modules[i]);
        struct Symbol {const char *name;void **target;};
        Symbol symbols[]={
            {"_ZN4IPMI6Client6ConfigC1Ev",(void**)&ipcConfig},
            {"_ZN4IPMI6Client6Config24estimateClientMemorySizeEv",(void**)&ipcEstimate},
            {"_ZN4IPMI6Client6createEPPS0_PKNS0_6ConfigEPvS6_",(void**)&ipcCreate},
            {"_ZN4IPMI4impl10ClientImpl7connectEPKvmPi",(void**)&ipcConnect},
            {"_ZN4IPMI4impl10ClientImpl10disconnectEv",(void**)&ipcDisconnect},
            {"_ZN4IPMI4impl10ClientImpl7destroyEv",(void**)&ipcDestroy},
            {"sceBgftServiceInit",(void**)&bgPublicInit},
            {"sceBgftServiceIntInit",(void**)&bgInit},
            {"sceBgftServiceIntDownloadRegisterTaskByStorageEx",(void**)&bgRegister},
            {"sceBgftServiceDownloadStartTask",(void**)&bgStart},
            {"sceBgftServiceDownloadGetProgress",(void**)&bgProgress},
            {"sceAppInstUtilInitialize",(void**)&appInit},
            {"sceAppInstUtilAppExists",(void**)&appExists},
            {"sceAppInstUtilGetTitleIdFromPkg",(void**)&pkgTitle},
            {"sceAppInstUtilAppInstallPkg",(void**)&pkgInstall},
            {"sceLncUtilLaunchApp",(void**)&launchApp},
            {"sceSystemServiceAddLocalProcess",(void**)&addLocal},
            {"sceSystemServiceRegisterDaemon",(void**)&registerDaemon}};
        for(auto &symbol:symbols) if(!*symbol.target) {
            void *address=nullptr;int r=sceKernelDlsym(modules[i],symbol.name,&address);
            if(r==0 && address){*symbol.target=address;log(symbol.name,(int)modules[i]);}
        }
    }
    if(mode=="lnc-init" && !lncInit) {
        OrbisKernelSwVersion fw={};fw.Size=sizeof(fw);
        if(sceKernelGetSystemSwVersion(&fw)==0 && (fw.Version&0xffff0000U)==0x09600000U) {
            for(size_t i=0;i<count && i<256;++i) {
                void *address=nullptr;
                if(sceKernelDlsym(modules[i],"sceSystemServiceLaunchApp",&address)!=0 || !address)continue;
                OrbisKernelModuleInfo info={};info.size=sizeof(info);
                if(sceKernelGetModuleInfo(modules[i],&info)!=0)continue;
                uintptr_t anchor=(uintptr_t)address;
                bool bounded=false,stateBounded=false;
                for(unsigned j=0;j<info.segmentCount && j<4;++j) {
                    uintptr_t base=(uintptr_t)info.segmentInfo[j].address;
                    size_t size=info.segmentInfo[j].size;
                    uintptr_t state=anchor+0x3bac0;
                    if((info.segmentInfo[j].prot&2) && state>=base && state-base<size && size-(state-base)>=24)stateBounded=true;
                    if((info.segmentInfo[j].prot&1) && anchor>=base && anchor-base<size &&
                       size-(anchor-base)>=0x991c+1)bounded=true;
                }
                if(!bounded || memcmp(address,"\x55\x48\x89\xe5\x41\x56\x53\x48\x83\xec\x20\x4c\x8b\x35\xee\x7b\x03\x00\xc5\xf8\x10\x05\xc6\x98\x02\x00\x48\x85\xd2\x49\x8b\x06",32) ||
                   memcmp((void*)(anchor+0x1290),"\x48\x8d\x3d\x29\xa8\x03\x00\xe9\x54\x08\x00\x00\x90\x90\x90\x90",16) ||
                   memcmp((void*)(anchor+0x12b0),"\x48\x89\xd1\x48\x89\xf2\x48\x89\xfe\x48\x8d\x3d\x00\xa8\x03\x00\xe9\x4b\x0f\x00\x00\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90",32)) {
                    log("LNC analyzed 9.60 entry fingerprints mismatch",-1);continue;
                }
                // Offsets derived from this console's module, not old web offsets.
                lncInit=(int(*)())(anchor+0x1290);
                directLaunch=(int(*)(const char*,const char**,LncLaunch960*))(anchor+0x12b0);
                // Async start has a fourth argument: outTicket. No completion wait.
                if(!memcmp((void*)(anchor+0x1830),"\x49\x89\xc8\x48\x89\xd1\x48\x89\xf2\x48\x89\xfe\x48\x8d\x3d\x7d\xa2\x03\x00\xe9\x18\x4e\x00\x00\x90\x90\x90\x90\x90\x90\x90\x90",32))
                    startLaunch=(int(*)(const char*,const char**,LncLaunch960*,int*))(anchor+0x1830);
                if(startLaunch && !memcmp((void*)(anchor+0x1850),"\x89\xfe\x48\x8d\x3d\x67\xa2\x03\x00\xe9\x62\x51\x00\x00\x90\x90",16))
                    launchAsyncResult=(int(*)(int))(anchor+0x1850);
                if(launchAsyncResult && stateBounded && !memcmp((void*)(anchor+0x98e0),"\x55\x48\x89\xe5\x53\x48\x83\xec\x18\x48\x8b\x1d\x10\xe3\x02\x00\x89\xf2\x48\x8d\x4d\xec\xbe\x41\x00\x03\x00\x45\x31\xc0\x45\x31\xc9\x48\x8b\x03\x48\x89\x45\xf0\xc7\x45\xec\x00\x00\x00\x00\x48\x8b\x07\xff\x50\x68\x85\xc0\x75\x03\x8b\x45\xec\x48",61))
                    lncState=(unsigned char*)(anchor+0x3bac0);
                log("LNC split result entry resolved",lncState?0:-1);
                log("LNC async entry resolved",startLaunch?0:-1);
                log("LNC analyzed entries resolved",0);break;
            }
        }
    }
    if(mode=="lnc-init" && lncInit && lncResult!=0) {
        static bool attempted=false;
        if(!attempted){attempted=true;lncResult=bgft_call("LNC initialize scope",[]{return lncInit();});log("LNC initialize returned",lncResult);}
    }
    if(mode=="worker-register") {
        if(!isWorker || lncResult!=0 || !registerDaemon){log("worker registration not ready",-1);return;}
        static bool attempted=false;
        if(!attempted){attempted=true;daemonResult=bgft_call("worker register scope",[]{return registerDaemon();});log("worker register returned",daemonResult);}
    }
    log("AppInst Initialize symbol",appInit?0:-1);
    log("AppInst Exists symbol",appExists?0:-1);
    log("AppInst PkgTitle symbol",pkgTitle?0:-1);
    log("AppInst Install symbol",pkgInstall?0:-1);
    if(appResult<0 && appInit && appExists){log("AppInst initialize begin",0);appResult=appInit();}
    log("LncUtil symbol",launchApp?0:-1);
    log("AppInst init",appResult);
    log("BGFT init symbol",bgInit?0:-1);log("BGFT register symbol",bgRegister?0:-1);
    log("BGFT start symbol",bgStart?0:-1);log("BGFT progress symbol",bgProgress?0:-1);
    if(mode=="bgft-init" && bgResult!=0 && bgInit) {
        if(!bgHeap.heap){bgHeap.size=1024*1024;bgHeap.heap=calloc(1,bgHeap.size);}
        if(bgHeap.heap){log("BGFT initialize begin",0);bgResult=bgft_call("BGFT init scope",[]{return bgInit(&bgHeap);});log("BGFT initialize returned",bgResult);}
    }
    log("BGFT public init symbol",bgPublicInit?0:-1);
    if(mode=="bgft-public-init" && bgPublicResult!=0 && bgPublicInit) {
        if(!bgPublicHeap.heap){bgPublicHeap.size=1024*1024;bgPublicHeap.heap=calloc(1,bgPublicHeap.size);}
        if(bgPublicHeap.heap){log("BGFT public initialize begin",0);bgPublicResult=bgft_call("BGFT public init scope",[]{return bgPublicInit(&bgPublicHeap);});log("BGFT public initialize returned",bgPublicResult);}
    }
    if(mode=="ipc-connect") {
        if(ipcTested){log("IPC probe already attempted; restart for control",-1);return;}
        if(!ipcConfig || memcmp((const void*)ipcConfig,"\xc5\xf8\x10\x05\x88\x63\x00\x00\x48\xb8\x00\x00\x00\x00\x01\x00",16) ||
           !ipcEstimate || memcmp((const void*)ipcEstimate,"\xb8\x00\x10\x00\x00\xc3\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90",16) ||
           !ipcCreate || memcmp((const void*)ipcCreate,"\x55\x48\x89\xe5\x41\x57\x41\x56\x41\x55\x41\x54\x53\x50\x48\x85",16) ||
           !ipcConnect || memcmp((const void*)ipcConnect,"\x55\x48\x89\xe5\x53\x48\x83\xec\x18\x48\x8b\x1d\xf0\xbc\x00\x00",16) ||
           !ipcDisconnect || memcmp((const void*)ipcDisconnect,"\x55\x48\x89\xe5\x41\x57\x41\x56\x53\x48\x83\xec\x18\x4c\x8b\x3d",16) ||
           !ipcDestroy || memcmp((const void*)ipcDestroy,"\x55\x48\x89\xe5\x41\x56\x53\x48\x89\xfb\x8b\x7f\x08\xe8\x0e\x3a",16)) {
            log("IPC symbols or analyzed entry fingerprints mismatch",-1);return;
        }
        ipcCompatible=true;ipcTested=true;
        alignas(32) unsigned char config[0x158]={};
        ipcConfig(config);
        memcpy(config,"SceBgft",8);
        uint64_t zero=0,capacity=0x8400;uint32_t one=1;
        memcpy(config+0x10,&zero,8);memcpy(config+0x1c,&one,4);
        memcpy(config+0x28,&capacity,8);memcpy(config+0x30,&capacity,8);
        size_t size=ipcEstimate(config);log("IPC estimated client bytes",(int)size);
        if(size!=0x1000){log("IPC unexpected client size",-1);return;}
        void *memory=calloc(1,size),*client=nullptr;
        if(!memory){log("IPC allocation failed",-1);return;}
        log("IPC create begin",0);int r=ipcCreate(&client,config,nullptr,memory);log("IPC create raw",r);
        if(r==0 && client){
            int service=0;log("IPC connect begin",0);
            r=ipcConnect(client,nullptr,0,&service);log("IPC connect raw",r);log("IPC service raw",service);
            log("IPC disconnect begin",0);log("IPC disconnect raw",ipcDisconnect(client));
            log("IPC destroy begin",0);log("IPC destroy raw",ipcDestroy(client));
        }
        free(memory);
    }
    log(("probe complete "+mode).c_str(),0);
}
static void handle(int client) {
    const uint64_t deadline=sceKernelGetProcessTime()+10000000;
    std::string req; char block[1024]; size_t split=std::string::npos;
    while(req.size()<8192 && (split=req.find("\r\n\r\n"))==std::string::npos) {
        int n=receive(client,block,sizeof(block),deadline);if(n<=0)return;req.append(block,n);
    }
    if(split==std::string::npos || split>4096){response(client,400,"{\"error\":\"headers\"}");return;}
    size_t first=req.find("\r\n"); std::string line=req.substr(0,first);
    size_t length=0; bool hasLength=false,authenticated=false,invalid=false;
    for(size_t pos=first+2;pos<split;) {
        size_t end=req.find("\r\n",pos);std::string h=req.substr(pos,end-pos);pos=end+2;
        auto colon=h.find(':');if(colon==std::string::npos){invalid=true;break;}
        std::string name=h.substr(0,colon),value=h.substr(colon+1);
        for(char &c:name)c=(char)std::tolower((unsigned char)c);
        while(!value.empty()&&value[0]==' ')value.erase(0,1);
        if(name=="transfer-encoding")invalid=true;
        if(name=="content-length") {
            if(hasLength||value.empty()||value.find_first_not_of("0123456789")!=std::string::npos||value.size()>4)invalid=true;
            else {length=std::strtoul(value.c_str(),nullptr,10);hasLength=true;}
        }
        if(name=="x-ps4remote-key")authenticated=value==key;
    }
    if(invalid||length>256){response(client,400,"{\"error\":\"request\"}");return;}
    if(line=="GET /v1/health HTTP/1.1") {
        response(client,200,"{\"protocol\":\"ps4remote-1\",\"version\":\"0.24\",\"foreground_required\":true,\"package_category\":\"gde\",\"background_verified\":false,\"big_app_id\":"+std::to_string(bigAppId)+",\"mini_app_id\":"+std::to_string(miniAppId)+",\"app_init\":"+std::to_string(appResult)+",\"install_available\":"+((appResult>=0&&pkgTitle&&pkgInstall)?"true":"false")+",\"launch_available\":"+((appResult>=0&&launchApp)?"true":"false")+",\"launch_attempts\":"+std::to_string(launchTestAttempts)+",\"launch_limit\":"+std::to_string(launchTestLimit)+",\"role\":\""+std::string(role)+"\",\"pid\":"+std::to_string(getpid())+",\"worker_result\":"+std::to_string(workerResult)+",\"daemon_register\":"+std::to_string(daemonResult)+",\"lnc_init\":"+std::to_string(lncResult)+",\"bgft_init\":"+std::to_string(bgResult)+",\"bgft_public_init\":"+std::to_string(bgPublicResult)+",\"bgft_register_available\":"+(bgRegister?"true":"false")+",\"bgft_task\":"+std::to_string(ownedTask)+",\"controller\":false,\"capture\":false}");return;
    }
    if(!authenticated){response(client,403,"{\"error\":\"authentication\"}");return;}
    if(!hasLength){response(client,400,"{\"error\":\"length required\"}");return;}
    while(req.size()<split+4+length){int n=receive(client,block,sizeof(block),deadline);if(n<=0)return;req.append(block,n);}
    std::string body=req.substr(split+4,length);
    if(line=="POST /v1/probe HTTP/1.1") {
        if(body!="resolve"&&body!="direct"&&body!="sandbox"&&body!="internal"&&body!="bgft"&&body!="bgft-init"&&body!="bgft-public-init"&&body!="dep-system"&&body!="dep-user"&&body!="dep-netctl"&&body!="dep-http"&&body!="dep-ssl"&&body!="dep-np"&&body!="ipc-connect"&&body!="credentials"&&body!="caps-test"&&body!="system-test"&&body!="lnc-init"&&body!="app-slots"&&body!="worker-register") {
            response(client,400,"{\"error\":\"unknown probe\"}");return;
        }
        probe(body);
        response(client,200,"{\"probe\":\""+body+"\",\"app_init\":"+std::to_string(appResult)+",\"launch_attempts\":"+std::to_string(launchTestAttempts)+",\"launch_limit\":"+std::to_string(launchTestLimit)+",\"role\":\""+std::string(role)+"\",\"pid\":"+std::to_string(getpid())+",\"worker_result\":"+std::to_string(workerResult)+",\"daemon_register\":"+std::to_string(daemonResult)+",\"lnc_init\":"+std::to_string(lncResult)+",\"bgft_init\":"+std::to_string(bgResult)+",\"bgft_public_init\":"+std::to_string(bgPublicResult)+"}");return;
    }
    if(line=="POST /v1/bgft-start HTTP/1.1" || line=="POST /v1/bgft-status HTTP/1.1") {
        if(ownedTask<0 || body!=std::to_string(ownedTask)){response(client,400,"{\"error\":\"task not owned by this session\"}");return;}
        if(bgResult!=0){response(client,503,"{\"error\":\"BGFT not initialized\"}");return;}
        int r=-1;
        if(line=="POST /v1/bgft-start HTTP/1.1") {
            if(bgStart){log("BGFT start begin",ownedTask);r=bgft_call("BGFT start scope",[]{return bgStart(ownedTask);});log("BGFT start returned",r);}
            response(client,200,"{\"code\":"+std::to_string(r)+",\"installation_verified\":false}");
        } else {
            BgftProgress p={};if(bgProgress)r=bgft_call("BGFT progress scope",[&]{return bgProgress(ownedTask,&p);});
            response(client,200,"{\"code\":"+std::to_string(r)+",\"error\":"+std::to_string(p.error)+",\"bits\":"+std::to_string(p.bits)+",\"transferred\":"+std::to_string(p.totalTransferred)+",\"total\":"+std::to_string(p.total)+",\"installation_verified\":false}");
        }
        return;
    }
    if(line=="POST /v1/bgft-register HTTP/1.1") {
        if(bgResult!=0||!bgRegister){response(client,503,"{\"error\":\"BGFT unavailable\"}");return;}
        if(ownedTask>=0){response(client,409,"{\"error\":\"session already has a task\"}");return;}
        std::string title=body.substr(0,9),hash=body.size()>10?body.substr(10):"";
        if(body.size()!=74||body[9]!=' '||!title_valid(title)||!hash_valid(hash)){response(client,400,"{\"error\":\"package request\"}");return;}
        std::string path="/data/ps4remote/"+hash+".pkg";
        unsigned char header[128]={};FILE *f=fopen(path.c_str(),"rb");
        if(!f){response(client,400,"{\"error\":\"missing package\"}");return;}
        size_t n=fread(header,1,sizeof(header),f);fclose(f);
        if(n!=sizeof(header)||memcmp(header,"\x7f" "CNT",4)||memcmp(header+0x47,title.data(),9)) {
            response(client,400,"{\"error\":\"package header title mismatch\"}");return;
        }
        int user=-1,r=sceUserServiceGetInitialUser(&user);
        if(r<0){response(client,503,"{\"error\":\"user unavailable\"}");return;}
        BgftStorage p={};p.params.user=user;p.params.entitlement=5;
        // BGFT runs outside our /data alias; use the canonical service path.
        std::string servicePath="/user"+path;
        p.params.id="";p.params.url=servicePath.c_str();p.params.name=title.c_str();
        p.params.icon="";p.params.scenario="0";p.params.option=0x10000;
        int task=-1;log("BGFT register begin",0);r=bgft_call("BGFT register scope",[&]{return bgRegister(&p,&task);});log("BGFT register returned",r);
        if(r==0 && task>=0){ownedTask=task;log("BGFT registered task",task);}
        response(client,200,"{\"code\":"+std::to_string(r)+",\"task\":"+std::to_string(task)+",\"started\":false}");return;
    }
    if(line=="POST /v1/worker-start HTTP/1.1") {
        if(body!="start" || isWorker){response(client,400,"{\"error\":\"receiver only\"}");return;}
        if(lncResult!=0 || !addLocal){response(client,503,"{\"error\":\"LNC not ready\"}");return;}
        if(workerAttempted){response(client,409,"{\"error\":\"worker creation already attempted; inspect port 12802 and worker.log\"}");return;}
        OrbisAppInfo info={};int r=sceKernelGetAppInfo(getpid(),&info);
        log("worker parent app query",r);log("worker parent app id",info.AppId);
        if(r!=0 || memcmp(info.TitleId,"RMTT00001",9) || ((uint32_t)info.AppId&0xff000000U)!=0x60000000U) {
            response(client,503,"{\"error\":\"parent identity mismatch\"}");return;
        }
        struct stat st={};
        if(stat("/app0/remote-worker.self",&st)!=0 || !S_ISREG(st.st_mode)) {
            response(client,503,"{\"error\":\"worker file unavailable\"}");return;
        }
        workerAttempted=true;
        LocalProcess960 params={};
        log("worker create unprivileged begin",0);
        workerResult=addLocal(info.AppId,"/app0/remote-worker.self",nullptr,&params);
        log("worker create returned",workerResult);
        response(client,200,"{\"code\":"+std::to_string(workerResult<0?workerResult:0)+",\"local_process_id\":"+std::to_string(workerResult)+",\"worker_verified\":false}");return;
    }
    if(line=="POST /v1/launch-permission-probe HTTP/1.1") {
        if(body!="RMTS99999" || !isWorker){response(client,400,"{\"error\":\"worker missing-title probe only\"}");return;}
        if(lncResult!=0 || !lncState || !startLaunch || daemonResult!=0){response(client,503,"{\"error\":\"registered worker LNC required\"}");return;}
        static bool attempted=false;
        if(attempted || asyncLaunch.ticket>=0){response(client,409,"{\"error\":\"probe already attempted or launch outstanding\"}");return;}
        int user=-1;OrbisUserServiceLoginUserIdList users={};bool loggedIn=false;
        int r=sceUserServiceGetForegroundUser(&user);
        if(sceUserServiceGetLoginUserIdList(&users)==0)
            for(int i=0;i<4;++i)if(users.userId[i]==user && user>=0)loggedIn=true;
        if(r!=0 || !loggedIn){response(client,503,"{\"error\":\"foreground user unavailable\"}");return;}
        attempted=true;LncLaunch960 param={};param.user=user;int ticket=-1;ResultPoll result;
        log("missing-title permission probe RMTS99999",user);
        int submit=bgft_call("missing-title permission window",[&]{
            int sent=startLaunch("RMTS99999",nullptr,&param,&ticket);
            log("missing-title submit",sent);log("missing-title ticket",ticket);
            if(sent!=0 || ticket<0)return sent;
            uint64_t deadline=sceKernelGetProcessTime()+1000000;
            result=poll_result(deadline,[]{return sceKernelGetProcessTime();},[&](int &operation){
                auto mutex=(OrbisPthreadMutex*)(lncState+8);
                int locked=scePthreadMutexLock(mutex);if(locked!=0)return locked;
                int transport=-1;void *ipc=*(void**)(lncState+16);
                if(ipc) {auto table=*(void***)ipc;
                    using Read=int(*)(void*,unsigned,int,int*,void*,void*);
                    if(table && table[13])transport=((Read)table[13])(ipc,0x30041,ticket,&operation,nullptr,nullptr);
                }
                int unlocked=scePthreadMutexUnlock(mutex);
                return unlocked!=0?unlocked:transport;
            },[]{sceKernelUsleep(1000);});
            log("missing-title result transport",result.transport);
            if(result.transport==0)log("missing-title result operation",result.operation);
            log("missing-title polling complete",result.complete?0:-1);
            return sent;
        });
        response(client,200,"{\"code\":"+std::to_string(submit)+",\"ticket\":"+std::to_string(ticket)+",\"transport\":"+std::to_string(result.transport)+",\"operation\":"+(result.transport==0?std::to_string(result.operation):"null")+",\"complete\":"+(result.complete?"true":"false")+",\"polls\":"+std::to_string(result.polls)+",\"launch_verified\":false}");return;
    }
    if(line=="POST /v1/launch-async-result HTTP/1.1") {
        if(body!="RMTS00002" || !isWorker){response(client,400,"{\"error\":\"worker smoke test only\"}");return;}
        if(!launchAsyncResult || !lncState || lncResult!=0 || asyncLaunch.ticket<0){response(client,503,"{\"error\":\"no outstanding ticket\"}");return;}
        // Reading completion is deliberately unprivileged, even if it waits.
        int queriedTicket=asyncLaunch.ticket;
        // Same client, mutex, command and six arguments as the native wrapper.
        // Preserve transport status separately instead of collapsing it into result.
        int transport=-1,operation=0;
        auto mutex=(OrbisPthreadMutex*)(lncState+8);
        int locked=scePthreadMutexLock(mutex);log("async result lock",locked);
        if(locked!=0){response(client,503,"{\"error\":\"LNC mutex unavailable\"}");return;}
        void *ipc=*(void**)(lncState+16);
        if(ipc) {
            auto table=*(void***)ipc;
            using GetResult=int(*)(void*,unsigned,int,int*,void*,void*);
            if(table && table[13])transport=((GetResult)table[13])(ipc,0x30041,queriedTicket,&operation,nullptr,nullptr);
        }
        int unlocked=scePthreadMutexUnlock(mutex);log("async result unlock",unlocked);
        log("async result transport",transport);
        if(transport==0)log("async result operation",operation);
        int r=transport==0?operation:transport;
        if(transport==0 && asyncLaunch.completed(operation))log("async rejected control ticket retired",queriedTicket);
        response(client,200,"{\"code\":"+std::to_string(r<0?r:0)+",\"transport\":"+std::to_string(transport)+",\"operation\":"+(transport==0?std::to_string(operation):"null")+",\"ticket\":"+std::to_string(queriedTicket)+",\"launch_verified\":false}");return;
    }
    if(line=="POST /v1/launch-async-control HTTP/1.1" || line=="POST /v1/launch-async-scoped HTTP/1.1") {
        if(body!="RMTS00002" || !isWorker){response(client,400,"{\"error\":\"worker smoke test only\"}");return;}
        if(lncResult!=0 || !startLaunch || daemonResult!=0){response(client,503,"{\"error\":\"registered worker LNC required\"}");return;}
        bool scoped=line=="POST /v1/launch-async-scoped HTTP/1.1";
        if(!asyncLaunch.allowed(scoped)) {
            response(client,409,"{\"error\":\"requires completed rejected control and no outstanding attempt\"}");return;
        }
        int user=-1;OrbisUserServiceLoginUserIdList users={};bool loggedIn=false;
        int r=sceUserServiceGetForegroundUser(&user);
        if(sceUserServiceGetLoginUserIdList(&users)==0)
            for(int i=0;i<4;++i)if(users.userId[i]==user && user>=0)loggedIn=true;
        if(r!=0 || !loggedIn){response(client,503,"{\"error\":\"foreground user unavailable\"}");return;}
        LncLaunch960 param={};param.user=user;int ticket=-1;
        log("async launch foreground user",user);
        if(scoped) {
            asyncLaunch.begin(true);
            // Only the request submission is scoped, never the completion wait.
            r=bgft_call("async launch submit scope",[&]{return startLaunch("RMTS00002",nullptr,&param,&ticket);});
        } else {
            asyncLaunch.begin(false);
            log("async launch unprivileged control begin",0);
            r=startLaunch("RMTS00002",nullptr,&param,&ticket);
        }
        log("async launch submit returned",r);log("async launch ticket",ticket);
        asyncLaunch.submitted(scoped,r,ticket);
        response(client,200,"{\"code\":"+std::to_string(r)+",\"ticket\":"+std::to_string(ticket)+",\"launch_verified\":false}");return;
    }
    if(line=="POST /v1/launch-test HTTP/1.1") {
        if(body!="RMTS00002"){response(client,400,"{\"error\":\"only installed smoke title RMTS00002 allowed\"}");return;}
        if(lncResult!=0 || !directLaunch){response(client,503,"{\"error\":\"LNC not ready\"}");return;}
        if(launchTestAttempts>=launchTestLimit){response(client,409,"{\"error\":\"launch attempt limit reached\"}");return;}
        int user=-1,r=sceUserServiceGetForegroundUser(&user);
        log("launch foreground user query",r);log("launch foreground user",user);
        OrbisUserServiceLoginUserIdList users={};
        int listed=sceUserServiceGetLoginUserIdList(&users);bool loggedIn=false;
        if(listed==0)for(int i=0;i<4;++i)if(users.userId[i]==user && user>=0)loggedIn=true;
        if(r!=0 || !loggedIn){response(client,503,"{\"error\":\"foreground user is not logged in\"}");return;}
        ++launchTestAttempts;
        log("launch smoke attempt",(int)launchTestAttempts);
        LncLaunch960 param={};param.user=user;
        log("launch native parameter bytes",sizeof(param));
        log("launch smoke RMTS00002 direct unprivileged begin",0);
        // The LNC connection was initialized under SystemScope and restored.
        // Do not keep the system bit enabled across a potentially suspending call.
        r=directLaunch("RMTS00002",nullptr,&param);
        log("launch smoke returned",r);
        response(client,200,"{\"code\":"+std::to_string(r<0?r:0)+",\"app_id\":"+std::to_string(r)+",\"attempt\":"+std::to_string(launchTestAttempts)+",\"launch_verified\":false}");return;
    }
    std::string title=body.substr(0,body.find(' '));
    if(!title_valid(title)){response(client,400,"{\"error\":\"title\"}");return;}
    if(appResult<0){response(client,503,"{\"error\":\"AppInst unavailable\",\"code\":"+std::to_string(appResult)+"}");return;}
    int result=-1;
    if(line=="POST /v1/exists HTTP/1.1" && body==title) {
        int exists=0;result=appExists(title.c_str(),&exists);
        response(client,200,"{\"code\":"+std::to_string(result)+",\"exists\":"+(exists?"true":"false")+"}");
    } else if(line=="POST /v1/install-local HTTP/1.1" && body.size()==74 && body[9]==' ' && hash_valid(body.substr(10))) {
        if(!pkgTitle||!pkgInstall){response(client,503,"{\"error\":\"install symbols unavailable\"}");return;}
        std::string path="/data/ps4remote/"+body.substr(10)+".pkg";
        char actual[32]={};int isApp=0;struct stat st={};
        if(stat(path.c_str(),&st)||!S_ISREG(st.st_mode)||st.st_size<256){response(client,400,"{\"error\":\"missing package\"}");return;}
        result=pkgTitle(path.c_str(),actual,&isApp);
        if(result<0||title!=actual){response(client,400,"{\"error\":\"package title mismatch\"}");return;}
        // No uninstall/force-overwrite fallback. Report the platform's result.
        log("install begin",0);result=pkgInstall(path.c_str(),nullptr);log("install returned",result);
        response(client,200,"{\"code\":"+std::to_string(result)+",\"installation_verified\":false}");
    } else if(line=="POST /v1/launch HTTP/1.1" && body==title) {
        if(!launchApp){response(client,503,"{\"error\":\"launch symbol unavailable\"}");return;}
        int exists=0;result=appExists(title.c_str(),&exists);
        if(result<0||!exists){response(client,400,"{\"error\":\"title not installed\"}");return;}
        int user=-1;result=sceUserServiceGetInitialUser(&user);
        if(result>=0){LncAppParam p={};p.size=sizeof(p);p.user_id=user;p.LaunchAppCheck_flag=LaunchApp_None;
            log("launch begin",0);result=launchApp ? launchApp(title.c_str(),nullptr,&p) : -1;log("launch returned",result);}
        response(client,200,"{\"code\":"+std::to_string(result)+",\"launch_verified\":false}");
    } else response(client,404,"{\"error\":\"endpoint\"}");
}
static void stop_receiver(int reason) {
    log("receiver stop via system",reason);
    int r=sceSystemServiceLoadExec("exit",nullptr);
    log("receiver system exit returned",r);
    for(;;)sceKernelUsleep(1000000);
}
int main() {
    mkdir("/data/ps4remote",0777);
    logfile=fopen(logPath,"w");log("RECEIVER 0.24",0);
    log(role,getpid());
    log("key read begin",0);
    FILE *f=fopen("/app0/receiver.key","r");if(!f||fread(key,1,64,f)!=64)stop_receiver(2);fclose(f);key[64]=0;
    log("key read complete",0);
    log("network before service probes",0);
    log("user initialize",sceUserServiceInitialize(nullptr));log("net initialize",sceNetInit());
    int server=sceNetSocket("ps4remote",ORBIS_NET_AF_INET,ORBIS_NET_SOCK_STREAM,0);
    log("net socket",server);if(server<0)stop_receiver(6);
    int reuse=1;log("net reuseaddr",sceNetSetsockopt(server,0xffff,4,&reuse,sizeof(reuse)));
    sockaddr_in address={};address.sin_len=sizeof(address);address.sin_family=ORBIS_NET_AF_INET;address.sin_port=sceNetHtons(serverPort);
    int result=sceNetBind(server,(OrbisNetSockaddr*)&address,sizeof(address));log("bind receiver port",result);
    if((unsigned)result==0x8041010dU) {
        result=bgft_call("net bind system scope",[&]{return sceNetBind(server,(OrbisNetSockaddr*)&address,sizeof(address));});
        log("bind privileged returned",result);
    }
    if(result<0){sceNetSocketClose(server);stop_receiver(3);}
    result=sceNetListen(server,4);log("listen",result);if(result<0){sceNetSocketClose(server);stop_receiver(4);}
    for(;;){int c=sceNetAccept(server,nullptr,nullptr);if(c<0){sceKernelUsleep(100000);continue;}
        handle(c);sceNetSocketClose(c);
    }
}
