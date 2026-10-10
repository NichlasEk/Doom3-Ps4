# Fysisk0.17: testappen startade efter dialog, SIGSYS vid _exit

2026-10-09 14:05 UTC. Ny FTP-markör (saknades före startförsöket):
RMTS00002 install smoke 0.02 launched. Klog visar RMTS00002 spawn/EXEC,
PID156/appId0x6000290c, därefter SIGSYS12. RIP0x8000028bc i libkernel,
BrF0x402bd0 -> BrT0x8000028b0. Lokalt testapp-ELF har _exit@plt vid0x2bd0
(base0x400000), anropas av libc _Exit efter return från main. Stark direkt
koppling till testappens avslutning, inte krasch i receiver/startfunktionen.
Receiver hade redan återställt credentials (se separat readback uid1 auth0010,
attrs2000038000000000 capsff00). Detta verifierar START EFTER MANUELL DIALOG,
inte obevakad fjärrstart. Själva mottagaren upptar BIG_APP-platsen.

Klog artifacts/remote-receiver/klog/20261009-140541-e7881849.bin
Arkiv artifacts/remote-receiver/crash-017/{receiver.log,smoke.log}.
Coredump togs men konsolens crash reporter raderade den efter avbruten rapport;
/data/sce_coredumps var tom vid FTP-avläsning. Ingen dump kan analyseras nu.

Testapp0.03 byggd, samma titel RMTS00002. Byter return från main mot
sceSystemServiceLoadExec("exit",nullptr); om den returnerar loggas resultatet
och appen väntar i stället för att falla igenom till _exit. Markerar även
requesting system exit. PKG-validering passerade, fysisk exit ännu ej provad.
Paket/manifest: dist/PS4-Remote-Install-Smoke-0.03.pkg och
artifacts/remote-install-smoke-003/manifest.json. Ingen installation/overwrite
begärd, ingen ny receiver-version behövs för den här ändringen.
Nästa utvecklingsproblem kvar: automatiskt lämna BIG_APP-platsen eller
mottagare i bakgrunden; inte löst av testappens exitfix.

# Testapp0.03 fysisk återgång till meny verifierad

2026-10-09 14:25 UTC. Användaren startade manuellt och rapporterade svart
skärm följt av återgång till menyn. FTP-logg innehåller0.03 launched och
requesting system exit, ingen unexpectedly returned-rad.
Klog PID162: Kill for LoadExec =>0, NOTE_EXIT, därefter försök att spawna
ELF-path exit ger0x80aa001a och systemet återgår till ContentAreaScene.
Alltså praktiskt fungerande återgång utan föregående SIGSYS-krasch, men
LoadExec("exit") är här en exit-via-misslyckad-respawn, inte felfri spawn.
Ingen ny crash i detta körförlopp. Ingen ny fjärrstart prövad; manuell appstart.
Evidens artifacts/remote-install-smoke-003/physical-run.log samt
artifacts/remote-receiver/klog/20261009-142513-8ae5c1c6.bin.
Nästa arbete är fortsatt automatisk överlämning från foreground receiver till
målapp utan konsoldialog, alternativt bakgrundsmottagare. Exitfixen ensam
löser inte att två BIG_APP inte får köras samtidigt.

---
