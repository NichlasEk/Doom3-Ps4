# 0.20 physical trial – child process PASS, launch rejected

2026-10-09 15:07–15:09 UTC, PS4 9.60,192.168.32.229.

- Receiver0.20 responds on12801, PID183. LNC init0, system scope restored.
- AddLocalProcess unprivileged returns0x1300c3d1, parent app0x6000300c.
- Klog confirms child PID184 EXEC /app0/remote-worker.self in RMTT00001 sandbox.
- Worker HTTP12802 responds role worker/PID184. Startup bind normal0x8041010d,
  scoped bind0, restore0, listen0.
- Worker LNC init0 and RegisterDaemon0; both scopes restored. Three total worker
  scope restores (bind, init, registration) verified before any launch.
- Prior RMTS00002 marker archived locally and renamed remotely to
  install-smoke-rmts00002.log.before-020-150802.
- Worker direct unprivileged launch attempt1:0x8094000f in13ms.
  Valid logged-in foreground user0x169ec9d8;24-byte native buffer.
- No new smoke marker. No app start verified.
- Receiver control: same direct unprivileged launch, same user, attempt1,
  same0x8094000f in9ms. This excludes a worker-only failure for this experiment.
- Both HTTP servers still healthy after the two rejected calls. No suspension
  or crash observed for these calls. No successful app switch, so background
  survival during app switching remains UNTESTED. RegisterDaemon0 alone does
  not prove independence from the parent or foreground lifecycle.

The0.19 scoped/public route reached the app;0.20 unprivileged/direct route does
not. A permissions requirement is a hypothesis, NOT proven: both privilege
scope and entry/parameter format changed. Exact meaning of0x8094000f was not
established. Do not label it a confirmed permission error or ABI success.

Next discriminating work: inspect the service's0x8094000f return path and design
an explicit scoped worker launch with a restoration mechanism that still runs
if the launching thread blocks/suspends. Do not reintroduce a scope across a
potentially suspending foreground call without resolving that lifecycle issue.
No new package produced in this trial. User can leave current processes open.

Evidence:
- artifacts/remote-receiver/physical-020-{init,receiver,worker,worker-ready}.log
- physical-020-marker-before.log, physical-020-after-worker.log
- physical-020-final-{receiver,worker}.log and physical-020-final-health.json
- klog/20261009-150719-a4975f3a.bin (includes historical0.19/ShellUI messages;
  do not attribute those old crash messages to0.20)
- klog/20261009-150802-e2a596e8.bin
- receiver-jobs: health5ef87afddc9244a1bd5f422fbff1d37c;
  init13eea27a9ccc45fe9bddff47242c27c8;
  spawn851ad5fdc5a34a57827f706c62a0eb1b;
  worker health3fd5730df5b6479e8ef561a7a3bb4353;
  worker init230e03945d2445a1b7fa09d718854e5a;
  registration5ebe3d0a09fa4975b291e9fd720cb530;
  worker launch435e0b97bd034152a50714160eb34058;
  receiver launch106520897fd946a1b127364ad94df03e.
