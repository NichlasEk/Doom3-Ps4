# Receiver0.21: separate launch submission and completion

2026-10-09.0.20 remains live with worker PID184; health confirmed15:15UTC.
No new physical launch performed from0.20: it cannot expose the new API.

Analyzed the same console9.60 SystemService module, not guessed web offsets:
- sceLncUtilStartLaunchAppByTitleId at0x5dc0 (public anchor0x4590 +0x1830).
- Four arguments: title, argv, native24-byte params, outTicket(int*).
- Wrapper body0xabf0 validates title/params/outTicket and calls marshaller0xddb0.
- Marshalling uses command0x30040,24-byte params and a ticket output.
- Unlike synchronous body0x67a0, it has no subsequent completion-event wait.
- GetResultLaunchAppByTitleId at0x5de0 (+0x1850), takes int ticket, body0xaf50,
  command0x30041. Completion is always requested unprivileged.
- Firmware/bounds checks and exact32/16-byte entry fingerprints gate new APIs.
  Original launch/init fingerprints remain required too.

This removes the explicit synchronous completion wait from the privilege
scope, but does NOT prove submission itself cannot block or precede process
suspension. Do not promise unconditional restoration across crashes/suspension.
Only worker with successful RegisterDaemon and LNC-init can use the new routes.

## Test endpoints

All require auth and exact body RMTS00002. Client supports matching subcommands.
1. launch-async-control: submits unprivileged, once per worker session.
2. launch-async-scoped: same submission/params under SystemScope, once; only
   permitted if control submission returned exactly0x8094000f and no ticket is
   outstanding. Restoration and readback occur before HTTP response.
3. launch-async-result: queries only our stored ticket, unprivileged. No automatic
   polling/retry. A ticket is never silently cleared based on an ambiguous error.

If control returns a ticket, inspect log/marker/result instead of sending a
second start. If completion is NOT_ALLOWED, that might concern reading the
result; do not assume the original launch was rejected or retry automatically.
If submission times out, archive logs and stop. No reboot or repeated start.

## Physical sequence after installation

Capture klog. Verify0.21 receiver health, initializeLNC, verify restores, start
worker, verify0.21 worker PID/health, initialize worker LNC and RegisterDaemon,
verify restores. Archive prior smoke marker if present (it was absent after0.20).
Run control; inspect exact return and new logs. Run scoped submission only when
its prerequisite is satisfied. Confirm restoration BEFORE any completion query.
A fresh marker and healthy worker after app switch are the success evidence.

## Validation and delivery

Host SystemScope restore/failure tests passed. Python syntax and git diff check
passed. Receiver and worker emulator HTTP tests cover authentication, title,
role/readiness rejection of the three new endpoints; actual LNC is unavailable
in emulator. PKG validation passed. Package manifest and FTP readback evidence
are artifacts/remote-receiver/{manifest,delivery-021}.json.

No full remote launch success is claimed for0.21 until the physical test.
