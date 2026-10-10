# Fysisk fjärrinstallation verifierad – Receiver 0.15, RMTS00002

2026-10-09 11:34 UTC. Receiver 0.15 init=0. Ny testtitel RMTS00002
(Remote Install Smoke 0.02) saknades före försöket. PKG-validering och
FTP-staging med full SHA256-readback passerade. Registrering task535=0;
start=0; status code=0,error=0,6619136/6619136 bytes, bits403.
Klog knyter task00000217 till RMTS00002 och visar AppInstallApp=0 samt
ended(state=3,runstate=4,error=0). Installation tog cirka4 sekunder.
Installerat /user/app/RMTS00002/app.pkg återläst i sin helhet; hash matchar
85c6e21f7c9bb5ab57dc1dbda47135f2972279954a8559beaa6e2f027d211427.
Detta är fysisk bekräftelse på fjärrinstallation, INTE bara godkänt start-anrop.

Scope restore=0 efter BGFT-anropen. Separat credentials-probe bekräftar
original auth0010, attributes2000038000000000, capabilitiesff00, UID1/PID141.
Receiver svarar stabilt på health efteråt. Doom orört. Ingen testappstart ännu;
fjärrstart/controller/capture saknas fortfarande. Health.install_available är
äldre AppInst-indikator och false trots verifierad BGFT-installation. HTTP
installation_verified förblir konservativt false; separat verifieringsrapport
innehåller den samlade konsol-/readback-evidensen.

Evidens: artifacts/remote-install-smoke-002/{manifest,verification,installation-readback}.json
artifacts/remote-install-smoke-002/{init,after-install}.log
artifacts/remote-receiver/klog/20261009-113348-d1a916fb.bin
Register journal bfae9a87adbe4fde82ae88e4f5ac0465; start a2d98ca79146433d83d37593354b9bb9.
Status 915e17b24a774f2a879fad4fbbe2fb84; credentials f844e66e8ef14434b1b7260e06e1fdde.
Misslyckat tidigare testjobb532/RMTS00001 är inte borttaget. Den aktuella
sessionen äger535; ingen nyregistrering av samma titel behövs.
Nästa utvecklingssteg: fjärrstart (LncUtil/AppInst symboler ännu otillgängliga).
