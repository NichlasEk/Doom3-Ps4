// Own-process observation and guarded capability experiment. Layout reference: ps4-libjbc.
// Called only after a 9.60 firmware check; writes limited to the guarded capability or system-bit transitions.
#include "credentials.h"
#include <stddef.h>
static int heap_pointer(uintptr_t p) {
 return p>=0xffff800000000000ULL && p<0xffffffff00000000ULL && !(p&7);
}
static int observe(void *thread, uintptr_t *args) {
 struct CredentialObservation *out=(void*)args[1];
 out->entered=0x524d544352454431ULL;
 uintptr_t td=(uintptr_t)thread;
 if(!heap_pointer(td))return 22;
 uintptr_t proc=*(volatile uintptr_t*)(td+8);
 if(!heap_pointer(proc))return 22;
 out->pid=*(volatile uint32_t*)(proc+0xb0);
 if(out->pid!=out->expected_pid)return 22;
 uintptr_t cred=*(volatile uintptr_t*)(proc+0x40);
 if(!heap_pointer(cred))return 22;
 out->uid=*(volatile uint32_t*)(cred+4);
 if(out->uid!=out->expected_uid)return 22;
 out->auth=*(volatile uint64_t*)(cred+0x58);
 out->attributes=*(volatile uint64_t*)(cred+0x60);
 out->capabilities=*(volatile uint64_t*)(cred+0x68);
 out->complete=0x524d544352454431ULL;
 return 0;
}
__asm__("receiver_kexec:\nmov $11,%rax\nmov %rcx,%r10\nsyscall\nret\n");
extern long receiver_kexec(void*,void*);
long receiver_read_credentials(struct CredentialObservation *out) {
 return receiver_kexec((void*)observe,out);
}

// Compare-before-write, only the observed 9.60 capability transition and restore.
static int change_caps(void *thread, uintptr_t *args) {
 struct CapabilityChange *change=(void*)args[1];
 uintptr_t td=(uintptr_t)thread;
 if(!heap_pointer(td))return 22;
 uintptr_t proc=*(volatile uintptr_t*)(td+8);
 if(!heap_pointer(proc) || *(volatile uint32_t*)(proc+0xb0)!=change->expected.pid)return 22;
 uintptr_t cred=*(volatile uintptr_t*)(proc+0x40);
 if(!heap_pointer(cred) || *(volatile uint32_t*)(cred+4)!=change->expected.uid)return 22;
 if(change->expected.auth!=0x3800000000000010ULL ||
    change->expected.attributes!=0x2000038000000000ULL)return 22;
 if(*(volatile uint64_t*)(cred+0x58)!=change->expected.auth ||
    *(volatile uint64_t*)(cred+0x60)!=change->expected.attributes ||
    *(volatile uint64_t*)(cred+0x68)!=change->expected.capabilities)return 22;
 uint64_t old=change->expected.capabilities,new_value=change->replacement;
 if(!((old==0xff00 && new_value==UINT64_MAX)||(old==UINT64_MAX && new_value==0xff00)))return 22;
 *(volatile uint64_t*)(cred+0x68)=new_value;
 if(*(volatile uint64_t*)(cred+0x68)==new_value)change->applied=0x524d544341505331ULL;
 return 0;
}
long receiver_change_capabilities(struct CapabilityChange *change) {
 return receiver_kexec((void*)change_caps,change);
}

// flatz ShellCore + caps[0] bit 62 (ucred+0x60), preserving all other bits.
static int change_system(void *thread, uintptr_t *args) {
 struct CapabilityChange *change=(void*)args[1];
 uintptr_t td=(uintptr_t)thread;
 if(!heap_pointer(td))return 22;
 uintptr_t proc=*(volatile uintptr_t*)(td+8);
 if(!heap_pointer(proc) || *(volatile uint32_t*)(proc+0xb0)!=change->expected.pid)return 22;
 uintptr_t cred=*(volatile uintptr_t*)(proc+0x40);
 if(!heap_pointer(cred) || *(volatile uint32_t*)(cred+4)!=change->expected.uid)return 22;
 if(change->expected.auth!=0x3800000000000010ULL ||
    change->expected.capabilities!=0xff00)return 22;
 if(*(volatile uint64_t*)(cred+0x58)!=change->expected.auth ||
    *(volatile uint64_t*)(cred+0x60)!=change->expected.attributes ||
    *(volatile uint64_t*)(cred+0x68)!=change->expected.capabilities)return 22;
 uint64_t old=change->expected.attributes,new_value=change->replacement;
 if(!((old==0x2000038000000000ULL && new_value==0x6000038000000000ULL)||(old==0x6000038000000000ULL && new_value==0x2000038000000000ULL)))return 22;
 *(volatile uint64_t*)(cred+0x60)=new_value;
 if(*(volatile uint64_t*)(cred+0x60)==new_value)change->applied=0x524d544341505331ULL;
 return 0;
}
long receiver_change_system(struct CapabilityChange *change) {
 return receiver_kexec((void*)change_system,change);
}
