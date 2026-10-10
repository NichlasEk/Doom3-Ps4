# 0.22: completed control rejection permits scoped comparison

2026-10-09. Fix for the observed0.21 sequence: submission0/ticket0 followed
by completion0x8094000f. Klog attributed this to startLaunchAppByTitleId's
isSystemProcess check for worker PID191 (see REMOTE-ASYNC-021-PHYSICAL.md).

AsyncLaunchState now records a completed control NOT_ALLOWED and retires that
control ticket. The scoped comparison becomes eligible once, just as with an
immediate submission rejection. Ticket0 remains valid. Unknown errors and
successful completion do NOT open the scoped path. Scoped attempts cannot
repeat or retire their ticket through the control-rejection path. HTTP result
still reports the queried ticket even when it has just been retired.

Tests cover ticket0, delayed and immediate rejection, pending/unknown errors,
success and duplicate prevention. SystemScope restore/failure tests pass.
Receiver and worker emulator HTTP checks pass; PKG validates. Console behavior
of scoped async submission still awaits installation/physical test. Restore
must be confirmed in the log before separately reading completion; no guarantee
that request submission itself cannot block/suspend.

Delivery: /data/pkg/PS4-Remote-Receiver-0.22.pkg,6619136 bytes.
SHA256 48ec61c9d09ffa2954234c7ba2f133a6da8bde4581c32311017c9922aff330a7.
FTP readback record: artifacts/remote-receiver/delivery-022.json.

After user installs/starts0.22: capture klog; init receiver LNC; verify restore;
start worker; verify version/PID; init worker LNC/register daemon; verify restores.
Archive marker if present. Run launch-async-control, then launch-async-result.
If result is NOT_ALLOWED, corroborate same system-process rejection in klog,
check ticket-retired log and healthy worker. Then launch-async-scoped ONCE.
Inspect scope restore before result retrieval. Fresh marker plus live worker
after app switching are required to claim success. No blind retries.
