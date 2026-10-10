# Remote smoke 0.01: register/start fungerar, BGFT avvisar /data URL

2026-10-09. Receiver 0.14 fortsatt aktiv. Byggt separat testtitel RMTS00001,
Remote Install Smoke 0.01. Ingen receiver-key eller kernelcallback i testappen.
Vid eventuell start skriver appen /data/ps4remote/install-smoke-rmts00001.log,
väntar 3 sekunder och avslutas. Ingen grafik. Start har INTE provats.
Paket 6619136 B, SHA256 6a9be342c2f9787b3cbbbd571cf8d0a7cfccb42c185158192ca8b2f0b749ef27.
Byggverktyg tools/ps4remote/build-install-smoke.py. Separat manifest
artifacts/remote-install-smoke/manifest.json. PKG-validering passerade.

FTP: /user/app/RMTS00001 saknades före. Stage med full hashåterläsning OK.
Registrering returnerade code=0 task=532, start code=0. Status code=0 men
error=0x80990004, transferred=total=0. Klog anger exakt task 00000214:
invalid content_url : '/data/ps4remote/<sha>.pkg'. Inga installationsfiler
fanns under /user/app/RMTS00001 eller /user/appmeta/RMTS00001 efteråt.
Jobbet finns kvar som misslyckat; ingen radering eller blind omregistrering.
Receiver scope restored=0 efter register/start/status. Doom orört.

Källkontroll: ps4-ipi installer.cpp använder /user/data/; ezremote installer.cpp
523 konverterar uttryckligen /data/ till /user/data/. FTP-återläsning via
/user/data/ps4remote/<sha>.pkg matchade hela testpaketet.
https://github.com/0x199/ps4-ipi/blob/master/Internal%20PKG%20Installer/installer.cpp
https://github.com/cy33hc/ps4-ezremote-client/blob/master/source/installer.cpp

Receiver 0.15 ändrar ENDAST BGFT contentUrl till /user+lokalpath; lokal fopen
fortsätter /data. Bygge/PKG-validering och full FTP-återläsning OK.
Levererat /data/pkg/PS4-Remote-Receiver-0.15.pkg, SHA256
50e6d16df49de583ef9e23a2fe8d757b8c744e8821f415f02d0087cba6a0518e.
Manuell installation/start krävs. Fysisk sökvägsfix ännu ej prövad.
Klienten flaggar nu även bgft-status.error!=0 som platform_error, och skiljer
mottagna plattformsfel från okvitterade nätverksanrop. Verifierat mot task532.
Nästa test bör använda ny testtitel RMTS00002 för att undvika jobbkollision
med kvarvarande532; ändra både SFO/content-id och appens loggidentitet.

Evidens:
artifacts/receiver-jobs/2271029dd3b340cbb8f1bab3bc09d208.json (register532)
artifacts/receiver-jobs/936b663c7aa44cbf8018c3d16e32352f.json (start532)
artifacts/receiver-jobs/17dead14052d4f2fb95f4e62acfe97a3.json (statusfel)
artifacts/remote-install-smoke/register.log och after-start.log
artifacts/remote-receiver/klog/20261009-112531-95b5f861.bin
artifacts/remote-receiver/goldhen-ftp-delivery-015.json
