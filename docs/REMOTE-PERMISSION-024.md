# 0.24: live process permission check and missing-title probe

Read-only FTP analysis of this console's /system/vsh/SceShellCore.elf:
SHA256 b3b352a4a302909dab7ba5533203be826d4b204f8dbd9bcaa8693ddf1bd61cff.
Local ignored copy: artifacts/remote-receiver/console-modules/SceShellCore.elf.
No console module patched or redistributed in package.

Handler startLaunchAppByTitleId0x338240 gets session PID via vtable+0x18 at
0x3383db and calls helper0x683af0 at0x3383e2. Helper calls get_authinfo(pid,
output) via PLT0xe16bf0/GOT0x13d1828, then tests bit62 of output+8. It returns
1 when that bit is set and query succeeds; failure returns negative. Handler
requires1, otherwise emits isSystemProcess failure and0x8094000f. This is an
active auth query during handling, not merely a saved boolean from connection
initialization. Whether underlying kernel auth data is cached was not analyzed.
This supports early-restoration timing, but physical causal comparison remains.

0.24 adds worker-only authenticated launch-permission-probe, exact fixed title
RMTS99999. Host client refuses unless FTP listings of /user/app and /user/patch
succeed and contain no such title; absence verified this turn. Do not install
that reserved title. No arbitrary target supported.

The probe keeps SystemScope across async submission AND result polling, using
same native client/mutex and split transport/operation result as0.23. Pending
0x80020023 is retried until a1-second monotonic deadline. This bounds retries,
NOT the duration of an individual native IPC call. Unknown errors end polling.
Scope restoration/readback occurs before HTTP response. Once/session and no
outstanding normal launch ticket permitted. This targets a missing app so a
successful permission check cannot produce the known foreground-app suspension.

Expected discriminating result: operation changes from NOT_ALLOWED to a
missing-title/application error while restoration succeeds. Exact missing-title
code is evidence to collect, not an asserted expected constant. This does NOT
prove that a real app can start or that the worker survives an app switch.

Physical sequence after installing0.24: capture klog, receiver health/LNC-init,
verify restore, worker-start, worker health/LNC-init/RegisterDaemon, verify all
restores; run launch-permission-probe BEFORE any real-title launch (client
checks absent title). Inspect operation, transport, complete, polls, restore
and health. No retry after timeout; do not generalize privilege window to a
real launch without an independent suspension-safe restoration mechanism.

Host tests exercise pending transport, pending operation, completion, unexpected
error and deadline. SystemScope restore/failure tests pass. Emulator HTTP guards
and role/readiness tests pass for both roles; actual native probe remains
physical-only. PKG validates.
