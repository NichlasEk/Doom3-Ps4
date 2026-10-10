# Receiver0.18 – avgränsat miniapp-test för automatisk överlämning

2026-10-09. Fysisk0.17 LNC-start av installerad RMTS00002 stoppas i
checkExistingApp med0x80940010. Mottagaren är BIG_APP; testappen också.
Manuell dialog leder till start, men uppfyller inte obevakat fjärrmål.

Primärkälla Itemzflow App-Media-Assets/sce_sys/param.sfo har CATEGORY=gde,
APP_TYPE=0,ATTRIBUTE=0x12. Vi lånar enbart kategorivalet för ett avgränsat
försök, inte dess AUTH_INFO eller övriga attribut. Itemzflow länkar andra
bibliotek och har annan runtime; deras beteende bevisar inte vår kombination.
https://github.com/LightningMods/Itemzflow/blob/main/App-Media-Assets/sce_sys/param.sfo
Kategorin är beskriven som MINI_APP; fysisk bekräftelse krävs via klog.

Ändring0.18: CATEGORY gd->gde, samma titelRMTT00001, samma paid0010,
samma fungerande systembit/BGFT/LNC och tio startförsök. Ny autentiserad
probe app-slots läser GetAppIdOfBigApp och GetAppIdOfMiniApp. Health visar
senast avlästa raw IDs, package_category=gde, background_verified=false.
foreground_required fortsätter true konservativt tills bakgrundsdrift bevisats.
Ingen daemon, ingen systempartition skriven, inga skip-check-flaggor ändrade.

Bygge/PKG-validering och SFO-avläsning (gde,RMTT00001,00.18) passerade.
Emulator19probe modes, HTTP/auth/task-guards passerade; inte fysisk miniapp.
Levererat /data/pkg/PS4-Remote-Receiver-0.18.pkg (6619136 B), fullreadback SHA
00363a50cfa7474a343834ca0b93c6ebea59a7abd0812cdc9122cb197b6769ee.

Efter användarens installation/start:
1. Health0.18; samla klog för faktisk appType. app-slots, credentials.
2. Om känd credential-baseline skiljer sig: undersök, kringgå inte spärren.
3. Kontrollera testtitelRMTS00002 och version0.03 finns. Arkivera/byt namn på
befintlig körmarkör före nästa försök eftersom0.03 redan körts manuellt.
4. lnc-init och restorekontroll, launch-test en gång. Läs ny markör/klog och
health under/efter appen för att skilja start från fungerande bakgrundskanal.
5. Om fortfarande dialog/BIG_APP: gör inte identiska retries. Om miniapp
suspenderas trots start: riktig bakgrundsdaemon kan fortfarande behövas.
Fysisk installation av0.18 återstår. Ingen fullt automatisk start hävdas.

Itemzflow tree commit: 9126e4788eb8a9d9657b8096ec7e25eb3bc9ab8d; SFO blob: 48175a5af52aa73e2747ae4f312ee2a25764a12a
