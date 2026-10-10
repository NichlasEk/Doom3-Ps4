# Receiver 0.16 – förberett kontrollerat fjärrstarttest

2026-10-09. 0.15 health verifierat: BGFT OK, testappen RMTS00002 installerad,
launch_available=false. Ingen fysisk fjärrstart ännu.

Primärkälla: https://github.com/LightningMods/PS4-daemon-writeup/blob/main/writeup.md
beskriver LncUtil inuti libSceSystemService och nödvändig initialize. Gamla
web-offsets används INTE. Egen konsols modul hämtad read-only via FTP:
artifacts/remote-receiver/console-modules/libSceSystemService.sprx (privat).
NID-match mot SDK och explicita funktionsnamn ger:
- sceSystemServiceLaunchApp 0x4590 (#K)
- sceLncUtilInitialize 0x5820 (#l), delta +0x1290
- sceLncUtilLaunchApp 0x5840 (#l), delta +0x12b0
- sceSystemServiceInitializeForShellCore 0x40a0 (#o)
Publika launch-wrappern läser user_id vid +4 och konstruerar egen 24-byte
native parameter. SDK LncAppParam är32 byte och skickas INTE till rå LncUtil.
InitForShellCore initierar flera andra delsystem; vi använder enbart LncInit.

0.16 probe lnc-init: firmware9.60, resolve publik launch, module-segmentgräns,
match32/16/32-byteprologer vid ankare/init/rawlaunch. Först därefter anrop av
LncInit inom samma SystemScope som BGFT. Prologmatch är begränsad kontroll,
inte fullständig runtime-modulhash. Init högst en gång/process.
POST /v1/launch-test accepterar enbart RMTS00002, init=0 krävs, en gång/session.
CLI: python3 tools/ps4remote/receiver-client.py launch-test
Anropar publika wrappern med initial-user-param inom systemscope. Positivt
app-ID normaliseras till code0 och returneras separat. Inte bevis på körning.
Foreground-mottagaren kan pausas innan native call/restore återkommer;
RAII kör vid retur, inte medan processen är suspenderad eller vid krasch.
Testappen skriver /data/ps4remote/install-smoke-rmts00002.log och avslutar
normalt efter3s. Samla klog och kontrollera ny körmarkör viaFTP för startbevis.
Ingen blind retry vid timeout. Kontrollera receiverlog för restore när den kör.

Bygge/PKG-validering och emulator18probes passerade. Auth/fel-titel/LNC-not-ready
för nya endpointen verifierade403/400/503. Firmwareguard innebär ingen verklig
LNC-init/launch i emulator. FTP-fullreadback verifierad:
/data/pkg/PS4-Remote-Receiver-0.16.pkg
SHA256 3962999721b479089504bc22b8aa5375a8a18cc95eabffa1408c53b26ec93254
Kräver manuell installation/start. Därefter: health0.16, klog, probe lnc-init,
logg/restore, launch-test EN gång, FTP-markör och klog. Testappen är redan
installerad; ingen ny BGFT-installation behövs.

Analyserad modul SHA256: 234f03ee8a5a437511a7dffe5e0c0c2028ec7b510c7cd06dcf6b826a81fa3bb8
