# 0.24 physical: permission window reaches missing-title lookup

2026-10-10 07:34–07:35 UTC. Receiver PID213, worker PID214, version0.24.
Receiver init0, worker creation/init/register0. Three worker scope restores
verified before probe; client verified RMTS99999 absent in app/patch FTP lists.

launch-permission-probe: submission0/ticket0; transport0;
operation0x80940031; complete=true;61 polls. HTTP job elapsed243ms INCLUDING
FTP preflight; do not describe that as native launch latency.
Worker log proves scope enabled through result, then fourth restore0.
Klog20261010-073436-fcb3bec9.bin lines887–889 records launchApp(RMTS99999),
lnc_app_info.cpp:161 query0x80940031 and lnc_manager.cpp:407. Thus request
passed system-process permission checking and reached app-info lookup for the
known absent title. Both HTTP servers remain healthy. No actual app launched.

This supports keeping privilege through server processing, unlike0.22/0.23
which restored immediately after enqueue and got operation NOT_ALLOWED.
Not a perfect same-title A/B: previous tests targeted installed RMTS00002,
this targets absent RMTS99999. Exact scheduling/latency has not been measured.
Do not claim an end-to-end launch or suspension-safe restoration yet.

Next required work: preserve permission until server acceptance while ensuring
restoration even if real launch suspends the app/process. The missing-title
probe cannot prove that behavior. No arbitrary delay or indefinite privilege.
No extra real-title launch performed and no new package produced this turn.

Evidence:
artifacts/remote-receiver/physical-024-{init-receiver,ready-worker,final-worker}.log
artifacts/remote-receiver/klog/20261010-073436-fcb3bec9.bin (includes history)
receiver-jobs/6c461e0bee5d45849162a8e860cce034.json probe
receiver-jobs/0333756fa4a747fe8312c9b427edff9b.json worker health
receiver-jobs/e039343ad9c74d478a0795ba7da8bffc.json receiver health
