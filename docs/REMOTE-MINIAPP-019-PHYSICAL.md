# Fysisk0.19: bind fix PASS, fjärrstart men suspension och ShellUI-krasch

2026-10-09 14:51 UTC. NetInit0, socket14, reuse0, normal bind0x8041010d;
SystemScope bind0, restore0, listen0. Health0.19 svarar. Miniapp-nätverk
fungerar efter temporär systembit. app-slots före lnc-init ger båda0x8094000f,
så dessa värden är inte giltiga app-ID:n. LNC-init0, restore0.

RMTS00002 fanns före. Tidigare markör kopierad lokalt och omdöpt påFTP
med .before-019-tidsstämpel. Startförsök1 journal
5e63a7c7076943d29eac78db8661b8d8: HTTP timeout (ingen retry).
NY markör:0.03 launched + requesting system exit. Klog visar suspend
RMTT00001 PID171 och spawn/EXEC RMTS00002 PID174. Alltså fjärrstart nådde
appen; receiver pausades inne i synkrona startanropet. Logg slutar efter
system scope enabled: återställning EFTER LAUNCH ÄR INTE VERIFIERAD.
Processens credentials kan fortfarande ha temporär systembit medan den är
suspenderad. Inte beskriva som färdig/stabil obevakad fjärrlösning.

ShellUI fick samtidigt Unhandled Exception ArgumentOutOfRangeException:
Invalid User Id, actual -1. PID43 AppCrash och RESTART Shell UI. Testappen
avslutades via sin LoadExec(exit)-väg. Inga fler startförsök gjorda.
Misstänkt parameterproblem i publik wrapper: den skapar native24byteparam
med default -1 vid+4 och kopierar vårt user-ID till+12. Nästa måste analysera
nativeparam/korrekt API-ABI innan mer fysisk launch. Separat behövs riktig
bakgrundsdrift eller asynkron överlämning med återställd behörighet före paus.

Evidens artifacts/remote-receiver/physical-019-{boot,marker-before,smoke,after-launch}.log
Klog artifacts/remote-receiver/klog/20261009-145141-03761350.bin
Initjournal9ae89db6e72743269495ae8c795034e7; health e8c2ffaf85fa46b1acb456add0183a3f.
Ingen ny version byggd i detta prov.
