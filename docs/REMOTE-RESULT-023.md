# 0.23: separate result transport from operation status

9.60 native wrapper0xde70 calls client vtable+0x68 with command0x30041,
ticket and output int. It returns transport when nonzero; otherwise output.
Thus the old API collapsed two statuses.0.23 duplicates this small call sequence
using initialized state0x40050, client+16 and mutex+8. GOT resolution confirms
native lock/unlock are scePthreadMutexLock/Unlock (0x3c330/0x3c350).

Firmware, module executable bounds, writable state bounds, existing entry
fingerprints and exact61-byte result-wrapper fingerprint are required. No module
code is patched. Dispatcher remains serial and native mutex is preserved.
Result retrieval is unprivileged. HTTP/log distinguish transport and operation
(null when transport fails). Only transport0 with operation NOT_ALLOWED retires
the control ticket. Existing one-attempt comparison logic otherwise unchanged.

PKG validation, both roles' emulator HTTP/auth/readiness tests and host async
state tests pass. Real client/vtable path cannot run in emulator: physical test
required, no successful remote app launch claimed.

Install/start0.23 then same receiver/worker init/register/restore sequence.
Capture klog and archive marker if present. Control submission/result first:
- transport0, operation0x8094000f => server returned denied operation.
- transport nonzero => result retrieval/transport failure; no scoped retry.
If control permits comparison, scoped submission once, verify restore, then
split result again. Do not issue additional starts on ambiguous outcomes.

Package /data/pkg/PS4-Remote-Receiver-0.23.pkg,6619136 bytes.
SHA256 b1e22568fb5c1f210fb63a79873396e2e8426eea0d78b3782209b53d29cdd14e.
Delivery evidence artifacts/remote-receiver/delivery-023.json.
