# PS4 probe client v1

Host-controlled debugger backend for the user's PS4, complementing Receiver0.24.
Uses unmodified [ps4debug-NG1.3.2](https://github.com/Pharaoh2k/ps4debug-NG/releases/tag/1.3.2),
source commit f941e2d31f83d5e1a3b64690a46787703498171a (GPL-3.0).
The client implements its documented wire protocol; payload and extracted console
modules remain ignored local artifacts. The generic client does not expose kernel writes. The separate worker_scope.py
implements only the checked Receiver-worker system-bit transition on9.60.

## Current physical status

2026-10-10: official payload loaded successfully after the user restarted the
payload loader. Version1.3.2, firmware960 and platform4 answered on744.
Process list, identity and926 ShellCore map entries verified. A64-byte read of
ShellCore startLaunch matched the local extracted ELF exactly; evidence is in
`artifacts/ps4-probe/shellcore-read-verification.json`.

Fixed initial map decoding timeout: upstream `proc.h` uses packed58-byte
`<32sQQQH>` entries (name/start/end/offset/protection). Client tests:11 passed.
Live RPC now verified: Receiver PID217 started worker PID218 after LNC init.
Worker identity/title checked; getpid PLT at0x45cc90 matched16 local ELF bytes.
Installed RPC stub0x4000 once, called getpid and received218, matching independent
HTTP health before and after. Evidence: `artifacts/ps4-probe/worker218-getpid-rpc.json`.
Addresses are session-specific. Do not reinstall the stub in this process.
Standalone alloc/compare-write commands remain untested live. Actual app launch
and survival through app suspension remain unverified.

Earlier empty9090 connection caused a GoldHEN payloader error and likely stopped
the loader. Never repeat an empty BinLoader probe. The successful load was one
direct send; no reload is needed while744 answers.

## Bring-up (run from repo root)

User toggles GoldHEN's Enable BinLoader Server off/on. Do NOT scan9090 first.
Then:
```
python3 tools/ps4remote/probe/prepare_payload.py
python3 tools/ps4remote/probe/debug_client.py load
python3 tools/ps4remote/probe/debug_client.py version
python3 tools/ps4remote/probe/debug_client.py processes
```
Load sends one raw pinned payload directly to9090. Never auto-retry a sent
payload; first inspect744/klog. Debugger lives on744 and installs upstream's
kernel module/syscalls. Payload is official release (3171952bytes), SHA256
28829f48465d3dfdf97afc15f01edafd8802f8213c44635af6f21e0e8b48c8f6.
Local build not used: upstream build.sh requires Ubuntu24.04/GCC13.3.0; current
host is Arch and Docker socket inaccessible. The source was inspected, not built.

First real validation: identify worker by returned process info/title, inspect
maps, read a small mapped code range and compare to our locally built binary.
Only afterwards install RPC into our worker and test a harmless known return.
Do not inject into ShellCore as a first test. Treat native RPC return as a raw
value, never as proof of effect. A timed-out RPC may still be running.

## Operations

Every targeted command requires current PID and expected title ID. Fetch PID
and resolve addresses again after each restart; numeric addresses are not stable.
Example using a freshly discovered PID (replace214 when it changes):
```
python3 tools/ps4remote/probe/debug_client.py inspect --pid 214 --title RMTT00001
python3 tools/ps4remote/probe/debug_client.py read --pid 214 --title RMTT00001 --address 0xADDRESS --length 64 --output artifacts/ps4-probe/read.bin
python3 tools/ps4remote/probe/debug_client.py alloc --pid 214 --title RMTT00001 --length 16384
python3 tools/ps4remote/probe/debug_client.py rpc-install --pid 214 --title RMTT00001
python3 tools/ps4remote/probe/debug_client.py rpc-call --pid 214 --title RMTT00001 --stub 0xSTUB --address 0xFUNCTION --args 1 2
```
`write` additionally requires --input and --expected files with equal length;
current memory must equal expected before write and replacement is read back.
No automatic retries. One mapped region per transfer, at most1MiB. CLI read
output uses exclusive creation to avoid overwriting an earlier dump. Allocated
memory and injected RPC thread persist until target exit; do not allocate in a
loop. Threads/registers/breakpoints are backend capabilities, not yet exposed by
this first client. Memory maps and protocol validity checks cannot eliminate
races with target process exit/restart.

Each operation creates a timestamped JSON journal under artifacts/ps4-probe/jobs,
including failed/unknown outcomes. Dumps stay local and contents are not echoed
in chat. This backend is a local LAN debugger, not an Internet service; its
classic read/write/RPC protocol does not have receiver.key authentication.

Sources: pinned upstream PROTOCOL.md and debugger/source/proc.c. Protocol sizes
were checked against code because some documentation line numbers are stale.

## Physical remote Doom launch and first timing sample (2026-10-10)

`launch_doom.py --pid 218 --stub 0x4000` launched DOOM00001 through the existing
worker: submission0/ticket0, Doom PID221 observed, worker credentials restored
and read back through the independent debugger connection. Klog confirms Receiver
suspension. No new package, ShellCore injection, or manual app-switch confirmation
was needed. These PID/address arguments are session-specific, NOT defaults.

The script checks firmware, userland identity, kernel PID/name/UID/credential tuple,
worker PLT bytes, native launch fingerprint and logged-in user before submission.
It modifies only ucred+0x60 bit62 for our worker and restores in finally. Host or
network failure can still leave restoration unconfirmed; this is not a persistent
console-side watchdog. There is no automatic launch/RPC retry. A scratch allocation
and existing RPC stub remain until worker exit. First fingerprint preflight failed
before allocation/elevation; corrected an extra byte in the host fingerprint.

`sample_doom.py --pid 221 --seconds 30 --out <new-json>` performs read-only sampling
against symbols and code fingerprints in build/client-runtime/dude.oelf. It does
not pause the process. Reads are asynchronous, so individual fields are not a
transactional engine snapshot. `analyze_present.py <log>` computes CPU wall-clock
log intervals, NOT GPU execution time. See docs/REMOTE-DOOM-PROFILE-20261010.md.

## SDL input into running Doom

`doom_sdl.py --pid <verified-pid> --command 'testGUI guis/intro.gui'` queues
SDL2 keyboard/text events into the normal engine loop. SDL_PushEvent is documented
thread-safe. Physically verified command acceptance AND intro GUI identity on221.
This is the intro GUI test, not the full New Game path. A test GUI can later be
cleared through the `testGUI` console command. No generic controller or capture yet.
`--reuse-session` is required when reusing an existing artifact; it is only valid
for the same uninterrupted process, never after reboot/PID reuse. Native RPC return
alone does not prove input took effect. Do not automatically retry unknown events.

## Full replacement cycle verified with Doom0.14

See docs/CLOCK-014-PHYSICAL.md: SDL quit removed Doom221, but Receiver remained
suspended. A verified RPC thread in the debugger-host ShellCore activated Receiver
through native LNC. No ShellCore code or credentials were patched. AppInstUtil loaded
there successfully; own Doom package replacement was then completed through native
uninstall and existing Receiver BGFT. Saves/config were backed up and hash-preserved,
installed PKG fully hashed, and Doom0.14 launched as224. Engine wall-clock fix verified;
intro FPS remained about9. Resume/install helpers are session-bound experiments,
not general commands. Do not replay historical PID/address/state files.

## Remote gameplay controls and image feedback

`doom_sdl.py` now accepts bounded `--keys`, `--move`, `--point`, `--click`
and recovery `--keys ... --release`, alongside the existing `--command`.
`doom_capture.py` retrieves a real screenshot through the engine and FTP.
See [physical control results and usage](../../../docs/REMOTE-CONTROL-20261010.md).
