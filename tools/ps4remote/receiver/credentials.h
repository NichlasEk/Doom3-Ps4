#pragma once
#include <stdint.h>
struct CredentialObservation {
 uint64_t entered, complete;
 uint32_t expected_pid,expected_uid,pid,uid;
 uint64_t auth,attributes,capabilities;
};
#ifdef __cplusplus
extern "C" {
#endif
long receiver_read_credentials(struct CredentialObservation*);
struct CapabilityChange { struct CredentialObservation expected; uint64_t replacement, applied; };
long receiver_change_capabilities(struct CapabilityChange*);
long receiver_change_system(struct CapabilityChange*);
#ifdef __cplusplus
}
#endif
