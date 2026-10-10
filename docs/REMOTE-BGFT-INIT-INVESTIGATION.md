# BGFT init investigation, 2026-10-09

Live configuration read via FTP, kept only in ignored artifacts. No console
configuration changed. FTP enabled; BinLoader enabled in file does not establish
that port is listening. bgft_enabled=1 under DebugSettings.

Correction: GoldHEN documents 9090 as BinLoader, NOT HTTP RPI. Do not send HTTP
requests there. Prior refused connection is not evidence about an RPI service.
Source: https://github.com/GoldHEN/GoldHEN

Klog receive-only port 3232 works. Archived artifacts/remote-receiver/klog-investigation.bin
contains successful RMTT00001 install and launch with appVer 00.08:
BGFT task 00000206 ends error=0; AppInstallApp returns 0; then launch 00.08.
These historical records corroborate the user's manual GoldHEN installation,
NOT remote installation by our receiver. System BGFT itself is operational.
Reusable bounded capture: tools/ps4remote/collect-klog.py, tested physically.
Raw captures remain ignored; avoid exposing account/system details in summaries.

Compared cy33hc/ps4-ezremote-client source/main.cpp and source/installer.cpp:
same 1 MiB heap and sceBgftServiceIntInit as our code. Crucially, Installer::Init
is called BEFORE initialize_jbc. Thus earlier inference that libjbc must precede
BGFT init is not established by this working reference. Its AppInst module is
loaded before BGFT init, while our AppInst fails. Dependency/session/credential
requirements remain candidates; 0x80990050 alone does not distinguish them.
Source: https://github.com/cy33hc/ps4-ezremote-client

No new blind package generated. Next discriminating controls should include
public sceBgftServiceInit vs internal init, explicit service prerequisites,
and runtime credentials observation if a compatible read-only mechanism exists.
Do not blindly repeat failed init or change kernel credentials as diagnosis.
No package registered/started, no game data changed, receiver 0.8 left running.
