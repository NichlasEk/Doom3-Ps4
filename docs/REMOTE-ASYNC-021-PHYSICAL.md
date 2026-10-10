# 0.21 physical: submission returns ticket, result NOT_ALLOWED

2026-10-09 15:27–15:28 UTC. Receiver PID190 and worker PID191, both0.21.
Receiver LNC init0, scoped restores verified. Worker creation0x1320c401,
worker health12802, LNC init0, RegisterDaemon0. Worker bind/init/register
all three restore readbacks verified before launch.

Prior RMTS00002 marker absent. launch-async-control returned code0,ticket0
in14ms. launch-async-result returned0x8094000f in34ms. FTP confirms no new
smoke marker; both HTTP servers remain responsive. No observed suspension or
crash, but no app switch occurred and background survival remains untested.

The completed klog capture resolves the ambiguity: for PID0xbf (worker191),
SceLncService reports !isSystemProcess(pid=0x000000bf), and specifically
startLaunchAppByTitleId at lnc_util_ipc_server.cpp:1205 returns0x8094000f
because isSystemProcess(session) is false. Thus the server rejected this launch
request on the system-process check; this is not merely a result-read error.

The0.21 scoped route requires immediate control-submission0x8094000f. Since
control returned ticket0 and the rejection arrived as completion, that route
is ineligible. No scoped launch or repeated submission attempted. The next
version should recognize this completed rejection, retire the consumed ticket
and permit the same scoped submission. Keep separate submission and completion
statuses; do not infer launch success from ticket creation. SystemScope still
must restore after submission and before unprivileged completion retrieval.

Evidence:
- physical-021-init.log, physical-021-worker-ready.log,
  physical-021-after-worker.log in artifacts/remote-receiver.
- receiver-jobs/4125dc0876b44d1f874042a253f973d5.json submission.
- receiver-jobs/4f81d1fa516a4cfcb27ce77f3f6adeac.json result.
- health8a81995606544c7db253d8231ddffaa7 and e1d665363aff474d8f9cadacf294e16c.
- klog/20261009-152737-4627c20c.bin:63260 bytes; lines809–810 contain the
  decisive system-process rejection. Capture includes historical messages.

No new package built in this trial. Leave0.21 open for continued investigation.
