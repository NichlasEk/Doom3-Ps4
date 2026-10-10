#include <cstdio>
#include <sys/stat.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
int main() {
    mkdir("/data/ps4remote",0777);
    FILE *f=fopen("/data/ps4remote/install-smoke-rmts00002.log","w");
    if(f){fputs("RMTS00002 install smoke 0.03 launched\n",f);fclose(f);}
    sceKernelUsleep(3000000);
    f=fopen("/data/ps4remote/install-smoke-rmts00002.log","a");
    if(f){fputs("requesting system exit\n",f);fclose(f);}
    int result=sceSystemServiceLoadExec("exit",nullptr);
    f=fopen("/data/ps4remote/install-smoke-rmts00002.log","a");
    if(f){fprintf(f,"system exit unexpectedly returned: 0x%08x\n",(unsigned)result);fclose(f);}
    // Do not fall back to the _exit syscall that produced SIGSYS on hardware.
    for(;;)sceKernelUsleep(1000000);
}
