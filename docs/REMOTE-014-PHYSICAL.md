# Fysisk Receiver 0.14 – BGFT init lyckas, befintlig Doom stoppar register

2026-10-09 11:23 UTC. Health visar 0.14. Probe bgft + bgft-init:
BGFT initialize returned=0, scope enabled=0, scope restored=0.
Separat credentials visar original auth=0x3800000000000010,
attributes=0x2000038000000000, capabilities=0xff00, uid=1, pid=138.

Registrering av SHA-verifierat Doom 0.13 (38862848 B) returnerade 0x80990088,
task=-1. Även detta anrops scope restored=0. Inget start-anrop skickat.
Klog innehåller [BGFT] SCE_BGFT_ERROR_SAME_APPLICATION_ALREADY_INSTALLED;
strömmen kan innehålla historik, men detta stämmer med aktuell retur och
befintlig Doom-installation. ezremote source/installer.cpp 556 hanterar
0x80990088/15 genom uninstall+retry. Den vägen har INTE använts här.
Health efteråt: init=0, task=-1, receiver svarar. Ingen färdig fjärrinstallation.
Nästa icke-destruktiva test: separat liten testtitel som inte redan är installerad,
för att verifiera register/start/progress utan att avinstallera fungerande Doom.

Evidens:
- artifacts/remote-receiver/physical-014-init.log
- artifacts/remote-receiver/physical-014-register.log
- artifacts/receiver-jobs/df950cf69ccd4a13837a203854b229dd.json (init)
- artifacts/receiver-jobs/e2653c16496e4d54b280e24c5d2cfbf8.json (register)
- artifacts/remote-receiver/klog/20261009-112327-679b8891.bin
- artifacts/receiver-jobs/8cd4978ca056400a94660f53a58218eb.json (health)
Klienten markerar negativa plattformsreturer som unconfirmed även när svar
finns. Här är registreringsfelet uttryckligen mottaget; ingen blind retry.
