# 0.22 physical comparison: both completions NOT_ALLOWED

2026-10-09 15:51–15:53 UTC. Receiver PID194, worker PID195, both0.22.
Receiver LNC init0; worker created0x1330c421, worker init0/register0.
Receiver two and worker three scope restores verified before launch.

Control submission0/ticket0; completion0x8094000f. Ticket retired correctly.
Klog20261009-155153-547ed6bf.bin lines498–499 confirms PID0xc3 rejected by
startLaunchAppByTitleId because isSystemProcess(session) is false.

Scoped submission at15:53:06 returned0/ticket1 in7ms. Worker log confirms
system scope enabled, call returned0, scope restored0, ticket1. Fourth restore
verified BEFORE unprivileged result query. Completion at15:53:13 again returned
0x8094000f. No fresh RMTS00002 marker. Both servers still respond after trial.
No observed new crash or app switch; background survival remains untested.

Unlike control, the scoped capture20261009-155304-b3d3bc1d.bin contains only
62 bytes and no detailed rejection. Therefore cannot identify whether scoped
failure came from submission processing or result retrieval using this capture.
Early restoration before asynchronous server processing is a hypothesis, not
proven. Another possibility is result-query permission. Do not claim that the
system bit itself is ineffective or that a delay will fix this.

Next useful diagnostic: distinguish transport return from operation result in
the completion wrapper and test bounded scoped completion retrieval of the
owned ticket, with restoration and explicit handling of blocking/suspension.
Do not introduce arbitrary sleeps or leave system credentials enabled across
unbounded waits. No further submission/retry performed; ticket1 is retained.
Current0.22 does not expose scoped completion. No new package built this turn.

Evidence in artifacts/remote-receiver/physical-022-*.log:
init-receiver,ready-worker,control-worker,scoped-worker,final-worker.
Marker retrievals return550 (absent); empty local marker files are not markers.
Jobs in artifacts/receiver-jobs:
- control9d180a2120724ebe941530f729d329dc/result5774fe40596447b084e43e4c26bae59c
- scoped92173b8a68014f718112afc76dd1c50a/result1023e03f3ac742a49f709a38d970ea0f
- final worker healthb7d763ef1cb64c568d4d705489cc9546
- final receiver health784de1244b7c4e08951b2320d50b7594
