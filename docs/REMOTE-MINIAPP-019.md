# 0.18 fysisk MINI_APP, bind-fel och sekundär _exit-krasch; 0.19 levererad

2026-10-09. Klog bekräftar CATEGORYgde -> SCE_LNC_APP_TYPE_MINI_APP,
version00.18, PID166. Receiverlog når key complete och bind12801=0x8041010d.
Därefter return3 från main -> libc _Exit -> _exit@plt vid0x59940,
BrF0x459940 -> libkernel0x8000028b0, RIP0x8000028bc, SIGSYS12.
Alltså miniapp startar; nätverksbind misslyckas och felavslutningen kraschar.
0.18 loggade inte NetInit/socketresultat så bind-felet ensamt ger inte full
orsakskedja. Inga credentials ändrades före felet. Coredumpkatalog tom.
Evidens artifacts/remote-receiver/crash-018/receiver.log och
artifacts/remote-receiver/klog/20261009-143105-4d808d71.bin.

0.19 behåller miniapp. Loggar userinit/netinit/socket/reuseaddr/bind/listen;
avbryter vid negativ socket. Vid exakt bind0x8041010d provas EN gång
samma bind inom befintlig SystemScope med firmware/baseline/readback/restore.
Detta är en hypotesprövning av behörighet, inte verifierad nätverksfix.
Startfel använder LoadExec("exit") i stället för return från main; om exit
returnerar loggas det och processen väntar. Credentials-restore-fail _exit(5)
är oförändrad separat fail-closed-väg. Inga permanenta credentialändringar.

Bygge/PKG/emulator19probes passerade; emulator verifierar ej miniapp-behörighet.
/data/pkg/PS4-Remote-Receiver-0.19.pkg levererad full FTP-hashreadback.
SHA8727c8d66870be830124c3fba3af35ec3c4e493b1a43968fed46fdf85456d666.
Nästa efter install/start: hämta receiverlog FÖRST, sedan health om tillgänglig.
Vid okänd miniapp-credentialbaseline: undersök logg, ändra inte blint guard.
