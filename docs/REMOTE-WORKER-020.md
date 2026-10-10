# Remote Receiver 0.20 – direct LNC and bundled worker

2026-10-09. Delivered to PS4 192.168.32.229 via anonymous FTP2121:
`/data/pkg/PS4-Remote-Receiver-0.20.pkg`, 6619136 bytes.
SHA256 `32abf5b07d1d5edbe1bb85397c5359c48116c34d5e145f5d595d95dde1b15312`.
Full FTP readback matched before .partial rename. Installation/start pending.

## Evidence and implementation

0.19 launched RMTS00002 but suspended the receiver inside the native call;
ShellUI crashed with invalid user -1. The public LaunchApp wrapper on this
console creates native parameters with user -1 at +4 and copies the caller's
+4 to native +12. This is a strong parameter hypothesis, not a proven sole cause.

0.20 calls direct LncUtilLaunchApp at public anchor +0x12b0, after existing
firmware9.60, module bounds and three entry fingerprint checks. Its buffer is
24 bytes, user at +4, remaining words zero. User must be foreground AND present
in the logged-in user list. Only RMTS00002 is allowed, ten attempts/session.
LNC initialization remains scoped; launch is unprivileged, so suspension cannot
strand the temporary system bit during this call. Whether unprivileged launch
succeeds with the initialized IPC connection still requires a physical test.

Bundled `/app0/remote-worker.self` runs the same dispatcher on12802 with role
worker and `/data/ps4remote/worker.log`; receiver stays12801/receiver.log.
POST worker-start with body start is authenticated, receiver-only, one creation
attempt/session. It checks own app title RMTT00001, miniapp ID and fixed SELF.
It calls AddLocalProcess unprivileged. No arbitrary paths or automatic retries.

The actual9.60 public AddLocalProcess wrapper treats arg3 as argv and arg4 as a
structure, unlike the writeup's prototype. LocalProcess960 reproduces the
wrapper's40-byte default mapping; field semantics beyond that are unknown.
The helper is not auto-started. Optional worker-register probe calls
RegisterDaemon once, scoped, only after successful LNC init in the worker.
Parent-child lifetime and suspension behavior remain unverified.

The stock fself converter crashes when linked only against kernel_sys because
it prepends kernel independently of its library index list. Further inspection
of the console's libkernel_sys.sprx shows its module AND library ABI names are
libkernel. build-worker-fself.py builds an isolated converter: keep libkernel
ABI names and change only the dependency filename to libkernel_sys.prx. No
shared SDK/tool modification. Worker OELF metadata was checked explicitly.
An initial wrong-name worker failed emulator imports; corrected worker passed.

## Checks

- PKG validation OK; includes receiver, helper, shared private key and icon.
- Host system-scope tests pass (restore, rejected writes, lost ack, exception,
  unsupported baseline, fatal restore failure).
- Host launch/local-process buffer byte-layout tests pass.
- Both receiver and corrected worker pass emulator HTTP/auth/title/readiness
  guards and20 diagnostic probe routes. Worker-start cannot recurse.
- Emulator does not verify proprietary LNC, real child spawning or background
  survival. No physical0.20 launch/worker success claimed.

## Next physical trial

1. User installs0.20 and starts receiver. Capture klog and health12801.
2. probe lnc-init, verify restoration and app identity, worker-start ONCE.
3. Inspect return, worker.log, klog and health12802. Do not retry after timeout.
4. If helper is alive: initialize its LNC, inspect credentials, optionally
   worker-register and verify restored credentials before launch.
5. Archive/rename the existing RMTS00002 marker BEFORE launching. Launch test
   from worker, capture both ports and klog during/after the foreground change.
6. Success requires a NEW marker and worker HTTP survival, not merely a positive
   return or an old marker. The test0.03 exits via LoadExec("exit"); its previous
   klog showed process kill followed by failed spawn of literal path exit.

Useful commands:
```
python3 tools/ps4remote/receiver-client.py health
python3 tools/ps4remote/receiver-client.py probe --mode lnc-init
python3 tools/ps4remote/receiver-client.py worker-start
python3 tools/ps4remote/receiver-client.py --port 12802 health
python3 tools/ps4remote/receiver-client.py --port 12802 probe --mode lnc-init
python3 tools/ps4remote/receiver-client.py --port 12802 probe --mode worker-register
```
Do not run the whole sequence blindly. Inspect evidence between steps.

Primary research:
- https://github.com/LightningMods/PS4-daemon-writeup/blob/main/writeup.md
- https://github.com/LightningMods/Itemzflow/blob/main/itemz-daemon/source/main.cpp

Actual console SystemService module SHA256:
`234f03ee8a5a437511a7dffe5e0c0c2028ec7b510c7cd06dcf6b826a81fa3bb8`.
Console modules and private keys remain local ignored artifacts.
