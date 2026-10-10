# 0.23 physical: transport succeeds, launch operation denied in both cases

2026-10-09 16:04–16:06 UTC. Receiver PID198, worker PID199. Both0.23.
Init/register pass; three worker restores before launch. Split-result fingerprint
resolved. Native mutex lock/unlock return0 in both result calls.

Control: submit0/ticket0; transport0, operation0x8094000f. Ticket retired.
Klog20261009-160452-103ec597.bin lines362–363 confirms PID0xc7 system-process
check denied startLaunchAppByTitleId.
Scoped: submit0/ticket1; fourth scope restore verified BEFORE result query.
Result: transport0, operation0x8094000f. Thus denied result retrieval is excluded
for these calls: the client received the launch operation's NOT_ALLOWED result.
No fresh smoke marker. Both servers still respond. No observed new crash or
app switch, no proof of background survival. Ticket1 remains recorded.
Scoped klog20261009-160558-4d9901da.bin1755 bytes has no detailed rejection line.

Early restore before async server permission check remains a hypothesis. Could
also involve permission snapshot/session semantics. Do not claim timing proved.
Next work must examine service permission lifetime/acknowledgment, or a bounded
launch architecture whose privilege restoration survives suspension. Do not
merely privilege result reads (transport has now been verified successful),
add arbitrary sleeps or leave credentials enabled across an unbounded wait.

Evidence: artifacts/remote-receiver/physical-023-{init-receiver,ready-worker,
control-worker,scoped-worker,final-worker}.log; marker retrieval550 absent.
Jobs in artifacts/receiver-jobs:
control8b7d85c7dc704710af3787b5ede89aee/result2382282bb56f4b5c992b6013775b23b8;
scoped11ea7213d13e43ebb1011492c06b89b6/resultebe6f72e049a4f9d80807fc13a4e35d5;
health747ea12b1533411887432c4d0361b296 and d62be2a523c746ca8e10231e32036765.
No new package built this trial. Leave0.23 open.
