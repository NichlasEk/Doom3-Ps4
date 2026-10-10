## 2026-10-10: clarified target and fresh-process first-map measurement

User means the loading screen AFTER the main menu, before the game begins,
not application startup. Restarted final0.17 remotely; fresh DoomPID266.
Started game/mars_city1 through SDL console after checking packed1/batch0/
parallel0. First-map116.627s: proc0.829s, game32.229s, media83.404s,
settle0.165s. Image window80.665s, read/inflate30.750s, upload40.648s,
including CPU-pack37.654s and wait1.939s. Inclusive stages must not be summed.
Same stable upload cost as earlier, variable I/O/game phase. No new optimization
or performance improvement claimed from this run; filesystem cache not reset.

Verified arrival intro via capture after loading; game tick/health100/cinematic1
confirmed. Engine log buffered until later; condump independently confirmed total.
Artifacts/texture-copy-017/fresh-start-result/{engine.log,engine.summary.json,
vulkan.log}; freshload018-settings.txt; freshloadresult018-settings.txt.
Capture artifacts/ps4-probe/captures/remote1791631001090937003.png retains the
known video inset/text corruption. No screenshot during engine load interval.

Next candidate: bounded reusable texel-offset tables keyed by full Gpa surface
context plus dimensions, to avoid repeated per-pixel surface address computation.
Must retain fallback, bounds, fences and byte-comparison tests; no implementation
or new PKG yet. Also investigate read/inflate variance separately from CPU-pack.
Current worker launch ticket13/scratch0x40000, credentials restored; ShellCore
resume ticket12. Receiver/worker suspended while Doom266 runs; don't query worker
completion until active. New SDL session belongs only to this live PID266.

## 2026-10-10: user confirms 0.17 improvement; startup investigation

User physically tested 0.17 and reports it became really good. Existing fix
5c750bc is pushed. Fresh read-only FTP snapshot is retained under
artifacts/texture-copy-017/startup-observation/ (engine PID260, final0.17 marker).
Do not reuse PID257 RPC state: the user has restarted Doom since our test.
This run reports first-map total87.681s, image65.800s, upload40.675s,
CPU-pack37.921s, file reads15.862s;2062packed/5generic. These support the earlier
upload result but are not a controlled new A/B comparison.

User requests faster loading of the game itself; clarification pending whether
application startup or map/save loading. Inspection confirms packed uploads are
currently gated by D3LoadActive(), so application startup still uses the generic
path. Candidate: measure InitGame phases and startup uploads, then explicitly
extend packed uploads to startup while retaining synchronous fences and fallback.
Do not infer menu-ready time from first QueuePresent (startup loading messages
also present frames). Current logs lack a separate menu-ready phase marker.
No new runtime change or package installed during this read-only investigation.

## 2026-10-10: static tiled texture uploads improve loading (0.17)

[Full results and limits](docs/TEXTURE-COPY-017.md). Same-package A/B/A/B Mars City
first-map runs:122.082/116.678/148.468/87.706s total. Upload itself is repeatable:
72.820/40.468/72.822/40.472s; waits32.91→1.9s with all fences retained. CPU-pack
cost37.64s remains inside upload. File reads15.7–34.8s vary independently; don't
claim a universal percentage from total times. No gameplay FPS gain established.
Packed2062/2067 images;5 use generic fallback. Both packed runs reached gameplay.

Final0.17 defaults r_ps4PackedTextures1; batch remains0. SHA256
12b0eabf59efc5cfe26730b413facbac8e1e21631a57e875850237396c486f60.
Full installed hash and config/save preservation verified. Earlier opt-in0.17
SHA7313eb59... and matching ELF retained in artifacts/texture-copy-017/opt-in/;
use matching ELF if controlling an old process after rebuilding. Current runtime
and dist are final default-on.0.16 rollback also retained. No assets committed.

Current DoomPID257, worker218 launch ticket12/scratch0x3c000, credentials restored.
Receiver217/worker218 suspended; ShellCore42 last resume ticket11. Completed
receiver-owned installation task577 retained. Verify live identity before using
session state; never poll worker completion while suspended. Final first-map
PASS:120.918s total/image84.785s/upload40.431s;2062packed/5generic.
Playable hangar screenshot inspected; native full-byte validation not claimed.
Defaults read back: packed1,batch0,parallel0,shadows1,fraction100.
Final stationary hangar baseline6.7065FPS/19.8315s; no gameplay FPS gain claimed.
Artifacts/texture-copy-017/, final/ and opt-in/. Emulator probe compares38.8MB of
GPU-written backing across18 shape/heap cases,36 updates, with guards; passed.
Physical byte-for-byte probe is not claimed. Full emulator map60frames passed.

## 2026-10-10: bounded upload experiment tested; kept opt-in

See [0.16 experiment](docs/UPLOAD-BATCH-016.md). B/A/B physical first-map loads:
150.580s batch /124.608s serial /122.895s batch. Last two image phases100.301s
versus100.401s: no clear image-load benefit. Wait calls2067→598, but wait time
32.877→32.204s. Do not claim the lower inclusive upload counter as a speedup;
flush work moves into other scopes. No general FPS gain established.

Final package0.16 has upload-batch=opt-in and default r_ps4UploadBatch0.
SHA36b9e5d2ee0a8967c94ca2520cc026a2d2fd0ef26b4d1766d774a34dc0995cd3;
full installed hash verified, config/save hashes preserved. Initial same-version
batch-on package SHA1eb3f211... is retained in artifacts/upload-batch-016/
batch-default-on/ with matching ELF. Dist/current runtime are final opt-in.
Corrected0.15 rollback and matching ELF retained. No assets committed.

Current DoomPID242, worker218 launch ticket7/scratch0x28000, credentials restored.
Receiver217/worker218 suspended; ShellCore42 RPC remains active, last resume
successful ticket6. Completed receiver-owned install task573 remains retained.
Do not poll worker completion while suspended. Session state is only valid for
this continuously running console; verify identity and RPC go before reuse.
Final runtime defaults read back: batch0, parallel images0, shadows1,fraction100.
Final first-map PASS:150.174s total/image116.501s after package replacement,
matching initial batch-on run150.580s/image116.568s. Slow reads recur on both
first-after-install runs; cache/storage effect plausible, not isolated.
Artifacts/upload-batch-016/final/ and final-check2/.

## 2026-10-10: optimization controls and load profile completed

Current installed build is corrected0.15, Doom PID230, actual profiler code
fingerprint verified against local dude.oelf. Map first load122.608s; image phase
100.768s; texture upload73.390s includes submit/wait32.907s. Image program22.107s
includes file read/inflate18.013s. Hash0.851s, mip generation0.437s. Inclusive
counters: do not sum all stages. Prioritize bounded upload batching/preparation;
parallel image loading remains off, synchronization unchanged. No FPS fix yet.
0.14 A/B/A/C/A hangar:6.651/8.575(no shadows)/6.660/7.469(50% dimensions)/6.605FPS.
Original settings restored. Full results: [0.15 investigation](docs/OPTIMIZATION-015.md).

Packaging mistake corrected: initial0.15-labelled PKG ran stale0.14 eboot;
quarantined locally. Packager now checks marker in raw ELF, OELF and eboot.
Final installed PKG SHA d6746bab1237bd20fede8762d4825de1c650e9d86c67d319a46a53106d9bb225.
Final evidence artifacts/optimization-015/corrected/, backups and stable0.14
rollback retained. Receiver217/worker218 suspended while Doom230 runs;
worker last launch ticket3 (do not blindly read completion while suspended).
ShellCore RPC remains usable; always check resume submission AND HTTP health.
Doom230 SDL session exists. Old224/227 sessions are historical and unsafe to reuse.

## 2026-10-10: source pushed, fresh map and gameplay measured

Initial source/tool history pushed as 92d338b. Subsequent console-state fix
reads actual open/closed status instead of blindly toggling. Fresh Mars City
map start confirmed; arrival 17.175 FPS/29.810s, stationary playable hangar
6.624 FPS/44.838s. Doom0.14/PID224 remains running there, cinematic=0, health100.
First attempted map did not execute; its 9.948 FPS sample is excluded from
fresh-game results. New load is warm: 2.272s engine phases. Earlier first load
in same log:131.395s, including109s image loading. No cold-boot control yet.
See [measurements and caveats](docs/GAMEPLAY-014-MEASUREMENTS.md).

## 2026-10-10: remote gameplay input and screenshots verified

Doom 0.14/PID 224: remote SDL keyboard and mouse input now physically verified.
Closed PDA with Escape, moved with W, turned camera by 8.8 degrees; retrieved
real 1280×720 screenshots via FTP. No new PKG required. Tools:
`tools/ps4remote/probe/doom_sdl.py` and `doom_capture.py`.
Details, evidence, current-session caveats: [remote control](docs/REMOTE-CONTROL-20261010.md).
User reports better cutscene, but loading sequences still heavy; loading work
remains outstanding. Re-identify PID/session before further remote work.

# Doom0.14 installerad och testad helt på distans

2026-10-10 ca08:32UTC. Användaren bad bygga/installera/prova klockfixen.
Paket dist/Doom3-PS4-Playable-Alpha-0.14.pkg,38862848byte, SHA256
 af7aad6b183a13370e756ccfefc688dbf744ff43460db588cb7b64cd436ade95.
Installerad fullhash matchar; config/saves backupade och hashbevarade.
Doom PID224 kör nu normala menyn, guiTest=null verifierat. Receiver217/worker218
är åter suspenderade. Debugger744 fortsätter svara. Ny ShellCore-RPC-session
finns i artifacts/ps4-probe/shellcore-rpc-session.json (PID42/stub0x4000/scratch0x8000).
Den återaktiverade Receiver efter Doom-quit, utan manuell åtgärd. AppInstUtil laddad
även i ShellCore; native init fungerade. Inga ShellCore-credential/codepatchar.

Klock-ID4 fixat i motorns init+löpande läsning.0.14 intro:267frames/29.953s=8.914FPS,
engine-tid29.983s (rätt);0.13 var9.207FPS men engine-tid26.017s/29.978s.
INGEN FPS-vinst i intro belagd. Varm meny29.960FPS, engine/host-tid matchar.
Nästa: instrumentera RoQ decode/staging/pacing, inte ta bort GPU-väntan.
OpenAL ombyggt samma källpatch men annan arkivhash; inte bitisolerat A/B.

Full process, begränsningar och evidens: docs/CLOCK-014-PHYSICAL.md.
Nya resume_receiver.py och replace_doom_preflight.py är sessionsbundna försöksverktyg,
inte generella återkörningskommandon. Uninstallspärr finns efter utfört0.13-byte.
Alla gamla PID221-/0.13-SDL-statefiler är historiska och får inte återanvändas.
Doom224-SDL-state finns; nya build/client-runtime/dude.oelf är matchande0.14.
Ingen commit/push, ingen USB-leverans begärd.0.13 bevarad lokalt för återgång.

---

# Intro fjärrstyrt via SDL; fel clock-ID upptäckt

2026-10-10 ca08:10UTC. Användaren valde intro/video. Doom221 kör testGUI
guis/intro.gui, startat via debugger→SDL_PushEvent→vanlig main-thread consoleinput.
Kommandot och guiTest-objektets namn verifierade. Ingen full New Game laddades.
Doom-RPC stub0x4000/scratch0x8000 i separat process221; statefil doom221-sdl-state.json.

Intro:9.207 FPS mätt mot hosttid,276frames/29.978s.104 presenteringscykler gav
109.1ms medel totalt,95.1ms före command-end,6.3ms submission→present.
Prioritera CPU-avkodning/staging/pacing; det är inte ett GPU-timestampbenchmark.

Konkret fel: Sys_MillisecondsPrecise använder clock_gettime(1), pga OpenOrbis
musl CLOCK_MONOTONIC=1. Native FreeBSD/Orbis clock1 är CPU-virtual, clock4 monotonic.
Live clockprobe bekräftar olika tidsgång. Äldre engine-PERF rapporterar60FPS där
host/Vulkan visar20FPS. Nästa fix ska välja native monoton tid även vid initial
baslinje; ändra inte clock-ID mitt i körning. Ingen Doom-kodfix gjord ännu.
All evidens och exakt tolkning: docs/REMOTE-DOOM-PROFILE-20261010.md.

---

# Doom startad på distans och första timing insamlad

2026-10-10 08:02UTC: DOOM00001 startad via worker218/RPC stub0x4000.
Doom PID221. Receiver217/worker218 suspenderades; hoståterställning av workerns
system-bit verifierad via debugger. Debugger744 och FTP2121 fortsatte fungera.
Ingen ny PKG. Se docs/REMOTE-DOOM-PROFILE-20261010.md för evidens och begränsningar.

Första30s-sampling:19.96 engine FPS. Färsk logg0.13, kod matchar lokal dude.oelf.
Scen ännu oidentifierad, användaren tillfrågad om intro/video eller gameplay.
Äldre motor-renderingtimers är0. Vulkanlogg: ca50ms/cykel, ca24ms före submission,
16ms submission→present och10ms blockerande present. INTE rena CPU/GPU-tider.
Inga Doom-prestandaändringar gjorda. Nästa: scenstyrd mätning och bättre deltimers.
Worker har en4KiB scratch vid0x8000; installera inte RPC igen blint.
Hoståterställningen är verifierad vid suspension men är ingen nätfelsoberoende watchdog.

---

# PS4 probe: första RPC fysiskt verifierad

2026-10-10 07:54UTC. Användaren startade Receiver0.24, PID217. Health svarar.
Worker saknades; första worker-start gav503 LNC not ready (ingen start utförd).
lnc-init gav0, därefter worker-start0 och workerhealth verifierade PID218.
Debugger info: remote-worker.self, /app0/remote-worker.self, RMTT00001.
Maps executable base0x400000. getpid@plt offset0x5cc90, live0x45cc90.
16 livebytes matchade build/remote-receiver/worker.elf fileoffset0x60c90.
RPC installerad EN gång i worker218, stub0x4000. getpid-anrop returnerade218;
oberoende HTTP health efteråt svarade worker/PID218. Detta verifierar ofarlig
RPC, inte appstart eller överlevnad vid appsuspension. Inga appstarter gjordes.

Evidens artifacts/ps4-probe/worker218.json, worker218-getpid-plt.bin,
worker218-rpc-install.json, worker218-getpid-rpc.json. Healthjournal
artifacts/receiver-jobs/49373a33febd4b54b929fde44a0aa603.json.
RPC-stub kvar i workern till processavslut; återinstallera inte i samma process.
Alla adresser/PID gäller denna körning och måste valideras innan nästa mutation.
Nästa större steg: återställning som klarar appsuspension och därefter riktig
fjärrstart. Ingen ny PKG eller extra fysisk åtgärd behövdes för detta RPC-test.

---

# PS4 probe laddad: fysisk läsning verifierad

2026-10-10 07:52UTC. Användaren startade om payload loader. Direkt uppladdning
av pinnad ps4debug-NG1.3.2 lyckades, 3171952 bytes. Ingen tom9090-kontroll.
Version/branding svarar1.3.2, firmware_raw960 (decimal), platform4.
Processlista och ShellCore PID42/titleNPXS20000 verifierade. Minneskarta926
poster.64 bytes vid live0x32b50240 (bas0x32818000 + startLaunch0x338240)
matchar exakt privata lokala SceShellCore.elf fileoffset0x33c240.
Evidens: artifacts/ps4-probe/shellcore-read-verification.json,
shellcore-inspect-fixed.json och shellcore-read.json. Klog
artifacts/ps4-probe/klog/20261010-074736-b7aea246.bin visar payload launched
successfully samt kernel/debugger loading.

Klientbugg fixad: proc_vm_map_entry är packed58 bytes, <32sQQQH>, inte64.
Första maps-timeouten var felaktig klientavkodning.11 hosttester passerar efter
rättningen. Inga processminnesskrivningar, alloc eller RPC har testats live.
Debuggerpayloaden själv installerar kernelmodul enligt upstream.
Receiver/worker finns inte i nya processlistan och12801/12802 svarar inte.
Nästa: starta befintlig Receiver0.24 för ofarlig RPC-verifiering i vår egen
worker; injicera inte första RPC i ShellCore. Ingen ny PKG behövs. Riktig
fjärrstart och suspension-safe återställning återstår. Debugger744 är aktiv;
ladda inte payload igen utan anledning. Adresser/PID måste verifieras på nytt.

---

# PS4 probe v1 förberedd; BinLoader behöver aktiveras igen

2026-10-10 07:43UTC. Användaren önskade bredare fjärrprobe. Ny klient finns i
[tools/ps4remote/probe/README.md](tools/ps4remote/probe/README.md).
Den använder ps4debug-NG1.3.2 official release, verifierad SHA256, källkod
pinnad till f941e2d31f83d5e1a3b64690a46787703498171a.11 hosttester passerar.
Funktioner: version/processer/info/maps/read/compare-write-readback/alloc/RPC.
Ingen debuggerfunktion fysiskt verifierad ännu. Receiver0.24 är oförändrad.

744 vägrar anslutning.9090 svarade vid TOM anslutningskontroll men vägrade
sedan payloadanslutningen. Klog artifacts/ps4-probe/klog/20261010-073930-636a672b.bin
visar GoldHEN payloader Error handling payload. Vår tomma kontroll verkar ha
stängt BinLoader. Ingen payload skickades (connect failed,0bytes).
UPPREPA INTE tom portkontroll mot9090. Klientens load skickar direkt verifierad
payload i samma anslutning, aldrig automatisk retry. FTP/receiver/worker lever.

Nästa: användaren slår av/på Enable BinLoader Server i GoldHEN. Därefter:
python3 tools/ps4remote/probe/debug_client.py load
python3 tools/ps4remote/probe/debug_client.py version
python3 tools/ps4remote/probe/debug_client.py processes
Fånga klog. Identifiera korrekt PID/title, läs maps/små data först. Ingen kernel-
eller ShellCore-manipulation behövs för första bring-up. Versionjobbet
c62eaf53effe4b20b6449a958788df6d bekräftar744 refused.

Ingen ny Receiver-PKG byggd. Lokalt debuggerbygge användes inte: upstream kräver
Ubuntu24.04/GCC13.3 och Docker-socket är inte åtkomlig. Official release hämtad
från upstream och full SHA256 verifierad i artifacts/ps4-probe/payload-manifest.json.
Paus/skydd mot paus-testet från tidigare plan ersattes av bred debugger-backend;
det är ännu inte utfört. Riktig appstart + suspension-safe återställning kvarstår.

---

# 0.24 fysisk: behörighetsprov passerar till app-lookup

2026-10-10 07:35UTC. Receiver213/worker214 lever. Saknad titel RMTS99999
verifierad via FTP. Probe submission0/ticket0, transport0, operation0x80940031,
complete=true,61 polls. Scope hölls genom resultatet och återställning0 verifierad.
Klog visar launchApp(RMTS99999) och app-info query0x80940031, alltså kom provet
förbi systemprocesskontrollen. Ingen riktig appstart/suspension testad.

Läs [0.24 fysisk evidens](docs/REMOTE-PERMISSION-024-PHYSICAL.md).
Resultatet stöder för tidig återställning i äldre async-prov; inte exakt samma
titel A/B. Nästa steg kräver återställning som klarar riktig appsuspension.
Ingen ny version byggd och inget extra launch-försök. Låt0.24 vara igång.

---

# 0.24 levererad: behörighetsfönster mot saknad testtitel

Read-only analys av konsolens ShellCore visar get_authinfo(pid) och bit62-test
inne i startLaunchAppByTitleId-handlern. Inte bara ett bool-värde från init.
Se [0.24 analys och prov](docs/REMOTE-PERMISSION-024.md).

Paket /data/pkg/PS4-Remote-Receiver-0.24.pkg FTP-återläst/verifierat.
SHA256 6f65baafc6df0db5528d467fcad505bfceb0cc5cb912b6b86559ac37a8be676e.
PKG, emulator båda roller, host polling/scope tester passerar. Installation
och fysisk test återstår. INGEN riktig appstart utförd denna tur.

Efter init/worker/register/restore: kör launch-permission-probe FÖRST.
Klienten kontrollerar att RMTS99999 saknas. SystemScope behålls över submission
plus resultpolling, återställs före HTTP.1s retry-deadline, native IPC-call kan
fortfarande blockera. Fast saknat title-ID undviker riktig appsuspension.
Inte applicera samma privilegieväntan på riktig app utan separat lösning.

---

# 0.23 fysisk: transport0, operation0x8094000f i båda försöken

2026-10-09 16:06UTC. Receiver198/worker199 svarar. Kontroll och scoped
startbegäran fick ticket0/1; båda resultatanrop har transport0 och nekad
OPERATION0x8094000f. Nekad resultathämtning är därmed utesluten för proven.
Scoped behörighetsåterställning verifierad före result-query. Ingen ny markör.

Läs [0.23 fysisk split-result](docs/REMOTE-RESULT-023-PHYSICAL.md).
Tidig återställning eller sessionens behörighetssemantik återstår att undersöka;
timing är inte bevisad. Inga fler starter, ticket1 kvar. Ingen ny version byggd.

---

# 0.23 levererad: separat transportstatus och startresultat

Ny diagnostik kräver installation/start0.23. Paket på /data/pkg, full FTP
återläsning verifierad. SHA256 b1e22568fb5c1f210fb63a79873396e2e8426eea0d78b3782209b53d29cdd14e.
PKG/emulator båda roller/host async-state tester passerar; fysisk test återstår.
Läs [0.23 resultatanalys](docs/REMOTE-RESULT-023.md). Ändringen använder samma
native mutex/client/command men bevarar två statusvärden separat. Ingen
förhöjd behörighet vid resultatläsning. Ingen fysisk start körd denna tur.

---

# 0.22 fysisk jämförelse genomförd: båda completion0x8094000f

2026-10-09 15:53UTC. Receiver194/worker195 lever. Kontrollens ticket0 nekades
med systemprocess-check bekräftad i klog. Scoped submission gav0/ticket1;
behörighetsåterställning verifierad FÖRE resultatfråga, som gav0x8094000f.
Ingen ny testmarkör/appväxling. Scoped klog saknar detaljerad felrad.
Tidig återställning respektive nekad result-query är hypoteser, inte avgjort.

Läs [0.22 fysisk jämförelse](docs/REMOTE-ASYNC-022-PHYSICAL.md).
Ticket1 behålls, inga fler starter gjorda. Nästa diagnostik bör separera
transportstatus/operationens resultat; undvik godtyckliga sleeps/scoped waits.
Ingen ny version byggd; båda0.22-processer kan lämnas igång.

---

# 0.22 levererad: kontrollens fördröjda nekande hanteras

2026-10-09. `/data/pkg/PS4-Remote-Receiver-0.22.pkg` FTP-återläst/hashverifierad.
SHA256 `48ec61c9d09ffa2954234c7ba2f133a6da8bde4581c32311017c9922aff330a7`.
Hosttester för async-tillstånd och scope passerar, båda roller klarar emulatorns
HTTP-tester, PKG validerad. Installation/fysisk0.22-test återstår.

Läs [0.22 ändring och testsekvens](docs/REMOTE-ASYNC-022.md).
Avslutad kontroll0x8094000f pensionerar kontrollticket och tillåter scoped
jämförelse en gång. Kontrollera klog/restore mellan varje steg; ingen blind retry.

---

# 0.21 fysisk: async submission0/ticket0, result0x8094000f

2026-10-09 15:28UTC. Receiver PID190 och worker PID191 svarar fortsatt.
Startbegäran återvände på14ms; resultatläsning gav0x8094000f på34ms.
Ingen ny RMTS00002-markör. Alla tre worker-scope-återställningar verifierade
före provet. Ingen appväxling eller bakgrundsöverlevnad verifierad.

Läs [fysiska0.21-resultat](docs/REMOTE-ASYNC-021-PHYSICAL.md).
Klog bekräftar orsaken: startLaunchAppByTitleId nekar PID191 eftersom
isSystemProcess(session) är false (lnc_util_ipc_server.cpp:1205).
Scoped route kördes INTE: dess krav är omedelbart nekad submission, medan
kontrollen gav en ticket och nekandet kom som completion. Nästa version behöver
hantera denna avslutade nekade begäran och därefter tillåta scoped jämförelse.
Ingen ny version byggd;0.21 kan lämnas öppen.

---

# 0.21 levererad: separat async startbegäran/resultat, installation väntar

2026-10-09. `/data/pkg/PS4-Remote-Receiver-0.21.pkg` FTP-återläst och verifierad.
SHA256 `8bd737f5378df16563a51c3e86490f151e5012c81cedcbefab97ad82da2f99e1`.
Båda roller klarar emulatorns HTTP- och åtkomsttester, PKG-validering och
hostens scope-tester passerar. Konsolen kör fortfarande0.20 (worker PID184).
Användaren behöver installera/starta0.21 före nya testet.

Läs [0.21 anropsanalys och testsekvens](docs/REMOTE-ASYNC-021.md).
Nya worker-only kommandon: launch-async-control, launch-async-scoped,
launch-async-result. Samma24-byteparam, enbart RMTS00002. Scoped test tillåts
endast efter kontrollens omedelbara0x8094000f utan utestående ticket.
Återställning sker efter submission, före separat unprivileged result query.
Ingen garanti att submission själv inte kan blockera/pausas: kontrollera logg.
Ingen fysisk appstart utförd i denna tur, inget nytt resultat utöver0.20.

---

# 0.20 fysisk: separat worker fungerar, direkt launch ger0x8094000f

2026-10-09 15:09 UTC. Receiver PID183/port12801 och separat worker
PID184/port12802 lever och svarar. Unprivileged AddLocalProcess lyckades;
worker LNC-init och RegisterDaemon gav0 med verifierad scope-återställning.
Direkt unprivileged start av RMTS00002 gav0x8094000f i BÅDA processer.
Ingen ny markör, ingen appväxling, ingen ny krasch observerad vid dessa anrop.
Bakgrundsöverlevnad under appbyte är fortfarande OTESTAD.

Läs [fysiska0.20-resultat](docs/REMOTE-WORKER-020-PHYSICAL.md).
Ingen ny installation behövs nu; låt processerna vara igång.0.19-krascher i
klog är historik. Exakt felbetydelse/behörighetsorsak är ännu inte bevisad.

---

# 0.20 levererad: direkt LNC + separat hjälpprocess, fysisk test väntar

2026-10-09 15:03 UTC. `/data/pkg/PS4-Remote-Receiver-0.20.pkg` överförd via
FTP och hela filen återläst/hashverifierad. SHA256
`32abf5b07d1d5edbe1bb85397c5359c48116c34d5e145f5d595d95dde1b15312`.
PKG-validering, hosttester och HTTP/emulatortester för BÅDA roller passerar.
Användaren har fått instruktion att installera och starta0.20.

Läs [0.20 testplan och ABI-fynd](docs/REMOTE-WORKER-020.md) först.
Receiver12801; medföljande worker12802 startas explicit med worker-start.
Direkt LNC24-byteparam med verifierad inloggad användare; launch och
hjälpprocessstart sker UTAN temporär systembit. Inget fysiskt0.20-prov ännu.
Stock fselfs kernel_sys-fel löst med isolerad converter som behåller ABI-namn
libkernel men väljer libkernel_sys.prx. Ändrar inte SDK eller systempartition.

---

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

---

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

---

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

---

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

---

# Fysisk0.17: installerad testtitel hittas; befintlig app blockerar

2026-10-09 14:03 UTC. Health0.17 attempts0/10. RMTS00002 finns viaFTP,
körmarkör saknas. LNC-init0 och restore0. Launch försök1 ger0x80940010.
Klog identifierar RMTS00002, categorygd, appVer00.02 och når
checkExistingApp där felet returneras. ShellUI öppnar LaunchConfirmScene /
ApplicationExitScene. Ingen körmarkör efter anrop. Detta är en annan spärr
än tidigare saknad app0x80940031. Receiver kör själv som BIG_APP.
Återställning verifierad i logg efter launch. Ingen blind retry skickad.
Användaren tillfrågad om synlig stäng-befintlig-app-dialog; inväntar svar.
Nästa: om OK valts, kontrollera markör och klog; hävda inte start utan bevis.
För helt obevakad start behöver livscykeln hantera att receiver upptar
foreground/BIG_APP-platsen; sannolikt separat bakgrundskomponent eller
systemets överlämning. Inte en fråga om fler identiska startförsök.

Evidens: artifacts/remote-receiver/physical-017-{init,launch}.log
artifacts/remote-receiver/klog/20261009-140323-8cb8af3c.bin
Launchjournal artifacts/receiver-jobs/d82157493ca841e8b93eb78a35b16312.json

---

# Receiver 0.17 – tio startförsök per process

Användaren bad uttryckligen om flera tillåtna starter för att slippa omstarter.
launch-test tillåter nu10försök/process, fortfarande endast RMTS00002 och
seriell dispatch. Inga automatiska retries. Försök räknas precis före native
anrop; felaktig titel/auth/oinitierad LNC förbrukar inget. Health/probe visar
launch_attempts/launch_limit och startsvar innehåller attempt. Loggnumrering.
Vid okvitterat resultat kontrollerar operatören markör/klog innan nytt försök.

Bygge/PKG-validering/emulator18probes samt nya healthfält passerade.
Fysisk upprepad start ännu ej verifierad. Native LNC-init var fysiskt OK i0.16;
start misslyckades då eftersom testappen saknades. RMTS00002 återinstallerades
med verifierat task539 och fullhashreadback. Behåll testappen inför nästa prov.
Levererat /data/pkg/PS4-Remote-Receiver-0.17.pkg viaFTP med fullhashreadback:
cab3cd165ffa58885eff6c60749c7c9b5d6fe48ec161e9e097fb73e14260d01f (6619136 B).
Nästa efter installation/start: health0.17, FTP kontroll titel/markörbaseline,
klog, probe lnc-init, kontrollrestore, launch-test och markör/klog/restore.
Ingen ny appinstallation behövs om RMTS00002 fortfarande finns.

---

# Fysisk 0.16: LNC init OK, testtitel saknades; återinstallerad

2026-10-09 12:18-12:20 UTC. Health0.16. LNC analyzed entries resolved=0,
LNC initialize=0, scope restore=0. launch-test EN gång gav0x80940031.
Ingen körmarkör före/efter. Klog visar tidigare GameWillDelete(RMTS00002)
och borttagning av jobb535/536; aktuell launchApp(RMTS00002) nekas i
lnc_app_info query. FTP bekräftade /user/app/RMTS00002 frånvarande.
Detta provar inte fjärrstart med installerad app! Init är däremot verifierad.
Credentials separat avlästa; original auth/attributes/caps återställda.

Testappen återinstallerad via0.16 BGFT: task539(0x21b), register/start0,
status0/error0,6619136/6619136. Installerat app.pkg fullreadback matchar
85c6e21f7c9bb5ab57dc1dbda47135f2972279954a8559beaa6e2f027d211427.
Loggar: artifacts/remote-receiver/physical-016-lnc-init.log,
physical-016-launch.log, physical-016-after-reinstall.log,
physical-016-reinstall-readback.json.
Klog:20261009-121851-5d78aefb.bin och20261009-121948-3e96ac5c.bin.
Launchjournal1c89f1e991f24e92bb1531641137d68f, reinstall3657e13cb54d48259da3528d9bddf959.

Nästa kräver STÄNG och STARTA samma Receiver0.16 (ingen ny PKG), eftersom
vår egen launchTestAttempted-spärr tillåter ett försök/process. Bevara testappen.
Efter restart: kontrollera RMTS00002 finns FÖRE launch, markörbaseline,
health0.16, klog, lnc-init, restore, launch-test en gång, markör/klog/restore.
Ingen automatisk retry har skickats och ingen ny mottagarversion byggts.

---

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

---

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

---

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

---

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

---

# Receiver 0.14 – scoped systembit runt BGFT

2026-10-09. Bygger vidare på fysiskt verifierad 0.13 IPC A/B/A2.
SystemScope aktiverar endast ucred+0x60 bit62 runt varje native BGFT-anrop:
internal/public init, register-by-storage, start och progress. Firmware 9.60,
känd auth/attributes/capabilities krävs. Full readback före anrop; jämför
PID/UID/auth/attributes/capabilities efter återställning. Vid oklar restore
avslutas processen med exit 5. Ingen HTTP eller generell service-probe körs
inne i scope. RAII hanterar normala returvärden och C++ exceptions; kan inte
återställa under en processkrasch. Credentials är processgemensamma; native
bibliotek kan ha egna trådar, även om receiver-dispatch är seriell.

Bygge och PKG-validering OK. Lokal tests/system-scope.cpp testar normal scope,
exception, nekad write, tappad write-ack och misslyckad restore (exit 5), samt
firmware/baseline-spärrar. Emulator: version 0.14, 17 probe modes och HTTP/
auth/task-spärrar OK. Emulatorn verifierar INTE verklig BGFT eller kernelwrite.
PS4 health före leverans: fortfarande 0.13, ingen BGFT-task.

Paket: dist/PS4-Remote-Receiver-0.14.pkg (6619136 bytes).
SHA256: 897a6d23faba54e79154ecb892bf0e163f8a27cabcc350f29c089a20bff8b94f
FTP-leverans dokumenteras i artifacts/remote-receiver/goldhen-ftp-delivery-014.json.
Nästa efter manuell installation/start: health måste visa 0.14, klog-insamling,
probe bgft, probe bgft-init EN GÅNG, hämta receiver.log och kontrollera både
native retur och scope restore, separat credentials-probe. Vid init=0 fortsätt
stegvis med redan SHA-verifierad Doom-paketregistrering och start. Ingen
automatisk ominstallation, uninstall eller upprepning av okvitterade anrop.
Fysisk 0.14 init/register/start/installation återstår.

---

# Fysisk Receiver 0.13: systembit öppnar BGFT IPC-anslutningen

2026-10-09 A/B/A2 i samma process:
A original: create 0, connect/service 0x80020001 EPERM.
B +ucred[0x60] bit62: create/connect/service/disconnect/destroy samtliga 0.
A2 återställt: create 0, connect/service åter EPERM.
Temporary readback och restoration verified båda 0. Separat credentials-probe
bekräftar original auth=0x3800000000000010, attributes=0x2000038000000000,
capabilities=0xff00, uid=1. Health svarar, ingen krasch.

Tydligt kontrollerat stöd för systembitens betydelse för just IPC-connect.
INTE bevis för BGFT init/register/start eller färdig installation; inget jobb
skickat. Nästa implementation: tidsbegränsad systembehörighet runt BGFT init
med återställning och verifiering; därefter stegvis pipeline. Ingen full libjbc
eller sandboxescape behövdes för det uppmätta anslutningssteget.

Logg: artifacts/remote-receiver/physical-013-20261009-111433.log
Journal: artifacts/receiver-jobs/4c4f21eeb54c47d2a27a92106bf47ea2.json

---

# Receiver 0.13 – flatz systembit A/B/A2, FTP-levererad

Användaren bad prova dokumenterade behörighetsprofiler. Valt först flatz:
https://flatz.github.io/ set_privileges_cb: paid ShellCore, caps[0] bit62.
Vår auth är redan ShellCore. Första 64-bitarsordet efter auth är ucred+0x60,
benämnt sonyCred i libjbc och attributes i vår logg. Föregående 0.12 ändrade
+0x68 och prövade alltså inte denna systembit.
0.13 probe --mode system-test gör samma A/B/A2 men endast +0x60 bit62:
0x2000038000000000 -> 0x6000038000000000 -> original. Auth/UID/caps behålls.
Kernelcallback kräver känd baseline och compare-before-write, readback samt
återställningskontroll. Exiterar om restore inte kan verifieras. Ett prov/process.
Ingen full libjbc/sandbox/root-profil ännu. Ingen installation i testet.

Bygge/PKG/emulator 17 modes passerade (firmwareguard, inte kernelwrite).
Lokal callback-harness verifierade avvisning vid fel PID/UID/auth/förbjuden
övergång/stale expected samt byteidentisk återställning av simulerad cred.
artifacts/remote-receiver/test-system-013.c.
/data/pkg/PS4-Remote-Receiver-0.13.pkg levererad via FTP med full hashåterläsning.
Fysisk 0.13-provning återstår. Förväxla inte Receiver 0.13 med Doom 0.13!
Nästa: efter användarens start, klog, health, bgft-symboler, system-test en gång,
FTP-logg och credentials-avläsning för separat restore-verifiering.

---

# Fysisk 0.12 capability-kontroll: ingen effekt, återställning verifierad

2026-10-09. A/B/A2 i samma process: alla tre IPC connect/service ger
0x80020001 EPERM. Create och destroy lyckas i alla tre. Temporary readback
bekräftar UINT64_MAX; restoration verified bekräftar original auth/attrs/UID/caps.
Separat credentials-probe efteråt och health utförda. Ingen krasch, installation
eller kvarlämnad capability-ändring. Slutsats: capability-fältet ensamt räcker
inte för att få just denna anslutning accepterad. Inte bevis att capabilities
saknar betydelse tillsammans med andra credentials.
Logg: artifacts/remote-receiver/physical-012-20261009-110445.log
Journal: artifacts/receiver-jobs/65d4b119993d4c9d9ba498f1674d54ce.json
Nästa analys bör skilja attribut/UID/sandbox/tjänstpolicy, inte upprepa caps ensamt.

---

# Receiver 0.12 – auktoriserat temporärt capability-prov, FTP-levererad

Användaren bad uttryckligen prova capability-ändring med återställning.
/data/pkg/PS4-Remote-Receiver-0.12.pkg finns på PS4, full SHA256-återläsning OK.
Ingen fysisk körning av ändringen ännu. Kräver installation/start av 0.12.
Nytt auth-skyddat probe --mode caps-test. Ordning:
- firmware 9.60, PID/UID samt känd auth/attributes/caps kontrolleras
- IPC original A, funktionsprolog-preflight måste passera
- endast ucred+0x68 ändras 0xff00 -> UINT64_MAX, med compare-before-write
- avläsning, separat IPC B, återställ 0xff00, avläsning verifierar auth/attrs/uid/caps
- IPC original A2; högst ett caps-test per process
Ingen auth-ID/UID/sandboxändring eller installation. Vid overifierad återställning
avslutas appen (_exit). Vid process/consolekrasch kan normal återställningskod inte
köras; provet avser endast egna processens credentials, inte global kernelpatch.

Bygge/PKG/emulator 16 probe-lägen passerade. Emulator verifierar firmwareguard,
inte kernel-write. Lokal C-harness med simulerade proc/cred-buffertar körde samma
callback med endast adressområdeskontrollen anpassad till host: fel PID/UID/auth,
förbjuden övergång och stale expected nekas; korrekt ändring+återställning gav
byteidentisk credentialbuffer. artifacts/remote-receiver/test-credentials-012.c.

Nästa fysisk test: klog, health 0.12, probe bgft (symboler), caps-test en gång,
hämta FTP-logg och verifiera restore innan slutsats. Inget automatiskt omförsök.

---

# Fysisk 0.11 – runtime credentials avlästa, kexec fungerar

2026-10-09: health bekräftar 0.11. credentials-provet kördes en gång.
Firmware 0x09600011. Syscall11 returnerar 0; callback entered och complete,
PID/UID-kontroller passerade. Observerat pid=129 uid=1,
auth=0x3800000000000010, attributes=0x2000038000000000,
capabilities=0x000000000000ff00. Inga credentials ändrades.
Det bekräftar att SELF paid faktiskt är runtime-auth-ID i denna körning och
att kernel-callbackgränssnittet fungerar. Det bevisar inte vilken capability
som behövs för BGFT. Nästa avgränsade kontroll kan jämföra capabilities före/
efter en tillfällig ändring, med återställning och avläsning; inte byta auth-ID
blint igen. Mottagaren fortfarande åtkomlig. Inget installationsjobb skickat.
Logg: artifacts/remote-receiver/physical-011-20261009-104403.log
Journal: artifacts/receiver-jobs/badf43e7f8fc4c1683c3da3d4989fe3c.json

---

# Receiver 0.11 – read-only credential observation FTP-levererad

/data/pkg/PS4-Remote-Receiver-0.11.pkg verifierad med full FTP-återläsning.
Inget installerat/startat av agenten. Nytt probe --mode credentials använder
libjbc:s syscall-11 kexec-ABI, men egen begränsad callback utan kernelwrites.
Referens: https://github.com/sleirsgoevy/ps4-libjbc (kernelrw.c/jailbreak.c).
Firmware måste vara 9.60 (sceKernelGetSystemSwVersion). En körning/process.
Kontrollerar kanoniska heappekare, proc PID mot getpid, UID mot getuid före
läsning av ucred +0x58/0x60/0x68 (auth/attributes/capabilities).
Inga process-/kernelbehörigheter ändras. Skickar resultat till egen userbuffer.
Fristående callback byggd freestanding, inga stackprotector/SIMD/red-zone,
disassembly kontrollerad. Ingen extern libjbc-kod inlänkad.

Bygge + paketvalidering + emulator 15 probe-lägen och felgrenar passerade.
Emulator provar firmware-avvisning, INTE callback/kernel-kompatibilitet.
GoldHENs syscall11-stöd i denna runtime är ännu okänt; om ej tillgängligt kan
anrop avvisas eller appen avslutas. Detta är observationsbygge, ingen fix ännu.
Nästa: klog-capture, credentials en gång, FTP-logg, bedöm faktisk auth/caps.
Om callback inte körs: stoppa där, inga blinda credential-writes eller loopar.

---

# Fysisk 0.10: IPC create OK, connect EPERM

Se docs/REMOTE-IPC-010-PHYSICAL.md. Raw connect/service = 0x80020001.
Klientskapande lyckas, destroy lyckas, mottagaren stabil. Inget installerat.
Nästa: runtime-identitet och IPC-accesskrav, inte fler minnes-/paketgissningar.

---

# Receiver 0.10 – separat IPC connect-diagnostik FTP-levererad

Paket /data/pkg/PS4-Remote-Receiver-0.10.pkg fullständigt FTP-återläst och
SHA256 verifierad. Ingen installation utförd av agenten. Ikon kvar.
Se docs/REMOTE-BGFT-960-ANALYSIS.md för analys och begränsningar.
Nytt probe --mode ipc-connect: native Config ctor, estimate, create, connect,
service-result, disconnect/destroy. Separat klient, ingen installation eller
systempatch. Symbol/funktionsprolog-skydd, ett försök per process.
Alla 14 probe-lägen och tidigare HTTP-felgrenar passerade i emulatorn;
IPC-framgångsväg ej testad (emulator saknar symboler). PKG validerad.
Nästa: användaren startar 0.10; klog-capture; bgft för symbolupplösning,
ipc-connect; hämta receiver.log. Dra slutsats från råa returvärden.

---

# BGFT 9.60 disassemblerad lokalt – IPC-fel avgränsat

Se docs/REMOTE-BGFT-960-ANALYSIS.md. FTP levererade BGFT som läsbar ELF.
Publik och intern init är wrappers runt samma 0xc20 och samma objekt!
0x80990050 genereras vid IPMI::Client::create eller följande virtuella klientanrop.
Parametrar/allokering har andra felkoder. Underliggande IPC-retur ännu okänd.
Binär/disassembly/reproduktionsskript hålls i ignorerade artifacts.
Ingen konsoländring, inget nytt paket. Nästa prov ska observera rått IPC-fel.

---

# Fysisk 0.9 – båda initvarianter misslyckas, beroenden laddas

2026-10-09, artifacts/remote-receiver/physical-009-20261009-102727/.
Health bekräftar 0.9. BGFT laddas. Publik init först: 0x80990050.
Intern init därefter: samma fel. dep-system/user/netctl/http/ssl/np laddas
alla med returkod 0; intern init testad efter varje, fortfarande 0x80990050.
Alla 15 probe-anrop returnerade och health svarade efter varje. Ingen krasch.
Ingen registrering eller installation skickad. Samma process hela provserien;
utesluter inte tillstånd som etablerats av det första misslyckade anropet.
Detta minskar stödet för att just dessa sex moduler saknas; bevisar inte
behörighetsfel. Nästa arbete bör analysera initens IPC/session/processkrav,
inte en ny variant av samma beroendeladdning.
Systemlogg fångad med tools/ps4remote/collect-klog.py till
artifacts/remote-receiver/klog/20261009-102714-3e3fd578.bin.

---

# Receiver 0.9 FTP-levererad – separata BGFT-init/beroendeprov

Paket /data/pkg/PS4-Remote-Receiver-0.9.pkg på PS4, full SHA256-återläsning
matchar. Ingen installation utförd av agenten. Användaren behöver installera/starta.
SHA256: 28c8e4bc10067d722b37daa83bcbee323fb42957e27f8bf3d243c4fc3ab7fc66

Nytt: bgft-public-init använder sceBgftServiceInit med separat 1 MiB heap och
separat bgft_public_init-status. bgft-init är kvar för IntInit. Registrering
kräver fortfarande lyckad intern init, publik framgång räcker inte.
Fjärrprov dep-system/user/netctl/http/ssl/np laddar ett beroende var med logg.
13 probe-lägen, efterföljande health och HTTP/BGFT-felgrenar passerade i emulatorn.
Emulatorn saknar BGFT; fysisk init-väg är inte verifierad.
Ingen authinfo/kerneländring. Processidentitet oförändrad från 0.8.

Fysisk provordning: börja klog-capture, health, bgft, bgft-public-init,
bgft-init. Vid fel: välj beroendeprov och kontrollera init igen. Kör inga
registreringar utan lyckad intern init. Ordning kan påverka processens tillstånd;
kontroller från nystart krävs innan beroende eller initordning kallas rotorsak.
Logga exakt försöksordning och gör inga automatiska mutationsomförsök.

---

# BGFT init undersökt vidare; systemlogg tillgänglig

Se docs/REMOTE-BGFT-INIT-INVESTIGATION.md. Port 3232 fungerar för klog,
nytt collect-klog.py verifierat. 9090 är BinLoader, INTE HTTP-RPI.
GoldHENs manuella installation av 00.08 lyckades enligt systemloggen.
Ezremote initierar BGFT före libjbc; runtime-behörigheter inte bevisad rotorsak.
Inget nytt paket/ingen configändring/ingen installation utförd.

---

# Fysisk 0.8 – BGFT init returnerar 0x80990050

2026-10-09: health bekräftar 0.8. BGFT laddas och alla fyra symboler hittas:
IntInit, IntDownloadRegisterTaskByStorageEx, DownloadStartTask, DownloadGetProgress.
Init med 1 MiB heap returnerar 0x80990050 (-2137456560). Ingen krasch,
health fungerar efteråt. Ingen registrering/start skickad eftersom init misslyckas.
Detta avgränsar felet till init före pakethantering; exakt orsak ej fastställd.
Undvik att använda PS5-källors etikett INVALID_SESSION som bevis för PS4-felet.
Logg: artifacts/remote-receiver/physical-008-20261009-101848.log.
Init-journal: artifacts/receiver-jobs/1ec5ef744e6743cc8f42460995177c41.json.
GoldHEN-alternativ: HTTP port 9090 nekade anslutning vid både GET / och
read-only POST /api/is_exists. Ingen tjänst tillgänglig där nu.
Nästa: undersök init-förutsättningar/runtime-credentials eller aktiverbar
GoldHEN Remote Package Install. Inget nytt paket byggt av denna provning.

---

# Receiver 0.8 FTP-levererad för GoldHEN

/data/pkg/PS4-Remote-Receiver-0.8.pkg finns på PS4. Full FTP-återläsning
matchar lokal SHA256. Ingen installation utförd; användaren installerar via
GoldHEN Package Installer med HDD-källa. Evidens: artifacts/remote-receiver/goldhen-ftp-delivery-008.json.

---

# Receiver 0.8 – BGFT pipeline klar för fysisk provning

Se docs/REMOTE-BGFT-008.md. Ingen installation utförd. USB saknas.
Paket: /run/media/nichlas/Kingston1TB/Doom3-Ps4/dist/PS4-Remote-Receiver-0.8.pkg
SHA256: 0a05388a29823cb34fb6f070239aaa3d7f643efcbecb0c5994d9ca631cfc1110

---

# Fysisk 0.7 – fjärrproven genomförda 2026-10-09

Version bekräftad över HTTP. Resolve + direct/sandbox/internal/bgft/internal/direct
kördes, med health och FTP-loggsnapshot efter varje laddningsprov. Samtliga
returnerade, mottagaren förblev åtkomlig.
- Direct AppInst: 0x80020002 (ENOENT), både före och efter BGFT.
- Sandbox AppInst: 0x80020008 (ENOEXEC).
- Internal AppInst: 0x80020008, även efter BGFT.
- Internal BGFT: 0 (lyckad modulladdning).
- app_init=-1, install_available=false, launch_available=false hela serien.

Evidens: artifacts/remote-receiver/physical-007-20261009-100858/.
Initial health/resolve: artifacts/receiver-jobs/e976fa48be9d4893b825ccdb49acd7ed.json
respektive 293a332cc28647789c5632153f2dd66b.json.
Ingen installation eller start skickad. BGFT init/symboler/jobb är ännu INTE
prövade och 0.7 har inget API för dem. Nästa meningsfulla implementation är
separata fjärrprov för BGFT symboler/init/jobbstöd utan AppInst-beroende.
Undvik att tolka modulladdning som installationsfunktion eller ENOEXEC som
bevis för en specifik behörighetsorsak. Ingen ny trapp-/USB-runda begärd.
Användaren föreslog namn i stil med PS4 Codex Remote Receiver.

---

# Receiver 0.7 – fjärrstyrda serviceprov, ersätter 0.6

Användaren vill inte göra fler turer för rena återställningsbyggen.
0.7 startar nätverket utan att ladda AppInst vid boot. Auth-skyddad
POST /v1/probe tillåter endast resolve/direct/sandbox/internal/bgft.
Klient: receiver-client.py probe --mode <läge>. Varje prov journalförs före
anrop; inga automatiska omförsök. Logg före och efter varje prov.
Alla fem prov + efterföljande health samt HTTP 403/503/400 passerade i emulatorn.
Fysisk tjänsteladdning ej verifierad. Prov kan krascha processen och kräva manuell
omstart, men flera ordningar kan testas utan paketbyte så länge appen överlever.
0.7 USB-levererad med full checksummeåterläsning och fsync. Stickan monterad.
Nytt prov: BGFT separat och AppInst efter BGFT. Resolve testar redan laddade moduler.
Ingen authinfo/kerneländring; genererad ikon kvar. Inte en bevisad installationsfix.

Undersökt ps4-ipi modules.cpp: använder libjbc före AppInst directload.
https://github.com/0x199/ps4-ipi
libjbc ändrar runtime sceProcType/sonyCred/sceProcCap och sandbox. Kräver kexec;
inte integrerat utan validerad kompatibilitet med användarens runtime.
https://github.com/sleirsgoevy/ps4-libjbc
TeamFAPS dumpverktyg matchar inte vårt headerformat (versionfält 0, inget keyset).
Ingen dumpdekryptering eller säker kraschadress erhållen.
https://github.com/TeamFAPS/PS4-RE-tools

Paket: /run/media/nichlas/Kingston1TB/Doom3-Ps4/dist/PS4-Remote-Receiver-0.7.pkg
SHA256: f6cda8d4dc3677b266a22d9793bc12b3dbc04f3327207b52ecc82dbbb9f68031

---

# Receiver 0.5 fysisk krasch; 0.6 återställning

Användaren rapporterade krasch. FTP-loggen innehåller endast RECEIVER 0.5.
Appen nådde main och första loggningen, men inget resultat från AppInst-laddning.
Det bevisar inte exakt kraschande anrop: nyckelläsning ligger emellan.
Konsolens rapport identifierar PS4 Remote Receiver 0.5. Logg, rapport,
manifest och binära dumpar sparade i artifacts/remote-receiver/crash-005-20261009-100145/.
Dumparna är inte avkodade; ingen säker feladress eller rotorsak fastställd.

0.6 tar bort explicit authinfo och återgår till 0.4:s SELF-parametrar,
behåller genererad ikon och lägger logg före/efter nyckelläsning och före
AppInst/SystemService-laddning. Återställningsbygge, INTE lösning på nekad tjänst.
Paketvalidering och emulator HTTP-test passerade. USB-leverans med full
SHA256-återläsning och fsync klar, stickan lämnad monterad. Fysisk test återstår.
Paket: /run/media/nichlas/Kingston1TB/Doom3-Ps4/dist/PS4-Remote-Receiver-0.6.pkg
SHA256: ddf97098f5b900a72ddd06dd470a03e7d93f66e927571726847b7d71776539bf

---

# Receiver 0.5 – authinfo och genererad ikon

0.4 saknade explicit SELF authinfo trots ändrat paid. OpenOrbis create-fself
skriver dessa separat. 0.5 behåller paid 0x3800000000000010 och lägger till
136-byte authinfo-profil från flatz/ps4_remote_pkg_installer Makefile.
Referenser: https://github.com/flatz/ps4_remote_pkg_installer/blob/master/Makefile
https://github.com/OpenOrbis/create-fself/blob/master/pkg/fself/FSELF.go
Detta är en testbar hypotes, inte bevisad runtime-behörighet på 9.60/Lapse.
Ingen kernelpatch tillagd. Inga install/start-kommandon skickade.
Explicit authinfo verifierad byte för byte i eboot.bin. Paketvalidering passerade.
Emulator health och HTTP 403/503/400 passerade. 0.5 USB-levererad, full
SHA256-återläsning matchar och fsync klar. Stickan lämnad monterad.
Ny genererad ikon: tools/ps4remote/receiver/assets/icon0.png, original och prompt
sparade bredvid. Fysisk 0.5-test återstår.
Paket: /run/media/nichlas/Kingston1TB/Doom3-Ps4/dist/PS4-Remote-Receiver-0.5.pkg
SHA256: cc045100d72bc6c6446cba232ae0864f94c78556de80564dc805ee445585bcc6

---

# Fysisk test 0.4 – 2026-10-09

Health bekräftar körande version 0.4. HTTP fungerar, app_init=-1,
install_available=false, launch_available=false. AppInst-laddning ger fortfarande
0x80020008 (ENOEXEC), identiskt med 0.3. Ändringen av SELF paid räckte inte.
Ingen installation/start skickad eftersom tjänsterna är otillgängliga.
Logg: artifacts/remote-receiver/physical-20261009-095638.log.
Health-journal: artifacts/receiver-jobs/914d31399cfa49efbd2da90639b7f93e.json.
Nästa steg: undersök runtime-behörigheter och loaderkrav innan nytt paket.
Användaren vill ha en cool AI-genererad ikon vid nästa version.

---

# Aktuellt – Receiver 0.4, 2026-10-09

Fysisk 0.3 HTTP fungerar. AppInst intern/sandbox-laddning returnerar
0x80020008 (ENOEXEC). SystemService laddas men launch-symbol saknas.
Logg: artifacts/remote-receiver/physical-20261009-095050.log.
Ingen installation/start begärd. 0.4 ändrar endast version och SELF paid från
0x3800000000000011 till ShellCore 0x3800000000000010 som ett avgränsat test.
Referens: https://flatz.github.io/ (PKG installation from internal HDD).
SELF paid kan vara otillräckligt för runtime ucred; fysisk verifiering krävs.
Paketvalidering och emulator HTTP/403/503/400 passerade.
0.4 USB-levererad med full SHA256-återläsning och fsync till SCUMMVM_PS4.
Stickan lämnad monterad. Fysisk 0.4-provning återstår.
Paket: /run/media/nichlas/Kingston1TB/Doom3-Ps4/dist/PS4-Remote-Receiver-0.4.pkg
SHA256: 86b850fc42e4a4859a0521259f44a1ab9044718bdb2d028be957e461db62852d

---

# Aktuellt 2026-10-09 – Remote Receiver 0.3 redo för fysisk provning

Fysisk 0.2 svarade på HTTP 12801 men app_init=-1 och LncUtil-symbol saknades.
Logg: artifacts/remote-receiver/physical-20261009-114235.log.
Svart skärm förväntad: ingen rendering implementerad. Ingen install/start skickad.

0.3 använder sceSysmoduleLoadModuleInternal för AppInst/SystemService,
AppInst-sökväg via sandboxord och symboluppslag i laddade moduler. Loggar
modulladdning, modulnamn och varje nödvändig symbols tillgänglighet.
Health har separata install_available/launch_available. Saknade installsymboler
blockerar inte exists; respektive operation avvisas innan anrop om symbol saknas.
Laddningsmönster referens: https://github.com/flatz/ps4_remote_pkg_installer
(main.c och module.c). Fysisk kompatibilitet ännu ej verifierad.

Paket: dist/PS4-Remote-Receiver-0.3.pkg, 6 619 136 byte,
SHA256 7c1ff25348423f02edaadd5089c7fca41e7704f77f8b41d83bddebd95da3a231.
Bygge + PkgTool-validering + emulator health/403/503/400 passerade.
0.3 USB-levererad till SCUMMVM_PS4 (UUID 90DA-404B). Full SHA256-återläsning
matchar, fsync klar, stickan lämnad monterad. Evidens: artifacts/remote-receiver/usb-delivery.json.
Nästa: installera/starta 0.3,
läs health och FTP-logg innan installation/start. Doom 0.13 oförändrad.

---

# Aktuellt 2026-10-09 – separat PS4 Remote Receiver 0.2

Mottagare byggd i tools/ps4remote/receiver/main.cpp. Paket:
`dist/PS4-Remote-Receiver-0.2.pkg`, titel RMTT00001, port 12801.
Instruktioner i tools/ps4remote/README.md avsnitt 0.2. Separat från Doom.
Egen nyckelskyddad klient receiver-client.py: health, exists, stage, install, launch.
Install använder FTP med full hashåterläsning + AppInstallPkg, launch använder
LncUtil. Returkod är inte bevis för färdig installation eller körning.
Ingen fysisk installation/start har ännu begärts. Första handpåläggningen:
installera mottagarpaketet från USB och lämna appen i förgrunden.

Verifierat: bygge, paketvalidering, fem uppackade runtimefiler, emulator-health,
403/503/400-grenar. Emulator saknar AppInst/LncUtil, fysisk 9.60/Lapse återstår.
Mottagaren saknar egen meny/rendering; startbild kan stå kvar. Kontrollera health
eller /data/ps4remote/receiver.log. Frånvarande tjänster ger fel, inga kernelpatchar.
Viktig begränsning: förgrundsapp, kan suspenderas när Doom startas. Ingen styrning/bild.
Nyckeln finns endast i ignorerade lokala artifacts/build/pkg, inte i källan.

Doom 0.13-paket förberett på PS4 /data/ps4remote/<SHA256>.pkg, 38 862 848 byte,
full återläsning matchar. Inget installerat av detta steg. Evidens:
artifacts/remote-receiver/staged-doom.json och artifacts/receiver-jobs/.
USB-resultat/manifest/tester i artifacts/remote-receiver/.
Tidigare dator-CLI på 12800 finns kvar för kompatibel RPI; blanda inte protokollen.
Ändringar fortsatt ocommittade.

---

# Aktuellt 2026-10-09 – PS4 Remote Tools 0.1

Användaren bekräftade firmware 9.60, Lapse via webb, ingen Remote Package
Installer/Chiaki installerad. Första datorverktyget finns i
[tools/ps4remote/README.md](tools/ps4remote/README.md).

CLI: doctor, collect, serve, install och status. Python standardbibliotek.
Fysisk FTP + byggkontrollerad insamling av 0.13-loggar passerade.
RPI-port 12800 svarar inte; ingen fysisk installation skickades.
Lokal testmottagare verifierar faktisk HTTP-klienttrafik, ranges/parallell HTTP,
PKG-manifest/hash, fel och loggidentitet. Sex tester passerade.
Evidens: artifacts/remote-tests.log, remote-doctor.json, remote-collect.json.
Loggsnapshot under artifacts/remote-runs/ har samma innehåll som tidigare
hämtning: bevisar inte nystartad körning. Inga runtime-/paketändringar.

Nästa arbete är konsolens mottagare för 9.60/Lapse. Fjärrstart, capture och
knappstyrning är ännu inte implementerade. Install-kommandot accepterar ett
jobb och sparar task-id; överföringsprogress räknas inte som verifierad
installation/start. Paketservern måste hållas igång under installationen.
Källändringarna i projektet är fortsatt ocommittade.

---

# Senaste observation 2026-10-09 – fysisk 0.13 och nästa verktygsarbete

Användaren: 0.13 fungerar, liten korruption i en video accepteras tills vidare.
Loggar hämtade via FTP, arkiverade med SHA-256 och analyserade.
Läs [PHYSICAL-013-20261009.md](docs/PHYSICAL-013-20261009.md): verifierad byggidentitet,
fortsatt Mars City/cinematic-körning, inga uttryckliga GPU-fel eller dump vid hämtning,
men kvarvarande långsamma intervall och ingen fastställd orsak till bildfelet.
0.13 är användbar med känt bildfel; 0.12 finns kvar som stabil jämförelse.

Nästa önskemål: fjärr-PKG, fjärrkörningar och assistentstyrning med bildåterkoppling.
Plan och verifieringssteg: [REMOTE-TESTING-NEXT.md](docs/REMOTE-TESTING-NEXT.md).
Inga sådana verktyg är ännu installerade/byggda. Ingen runtime-/paketändring
utfördes vid denna logggranskning. Tidigare källändringar är fortfarande ocommittade.

---

# Aktuellt 2026-10-09 – 0.13 tiled video staging

Användaren rapporterar fysisk 0.12 felfri med ljud, men hackande video.
0.13 behåller ljud och NIR-läckfix. Cinematic UpdateTexture2D packar CPU-pixlar
i befintlig tiled layout före staging, och skickar hela ytan med stora DMA.
Generell Vulkan-copy och textur-layout är oförändrade. Se
[VIDEO-UPLOAD-013.md](docs/VIDEO-UPLOAD-013.md).

Validering: GPU-probe videoupload, 513x509 med padding/flera uppladdningar,
921600/921600 skärmpixlar korrekta. PM4-livslängd och featuretester passerade.
Riktigt intro 600 frames med ljud och rent avslut i emulator. Mellanbilden
intro-001.png visar introtext/planet. Full fysisk videokorrekthet återstår.
Preliminär jämförelse mot sparad 0.12 (inte interfolierad A/B):
012 intervall ca 9–18 fps, 013 ca 13–24 fps. Senaste 500 kommandobuffertars
median 461133 -> 2166 dwords. Olika hastighet betyder olika filmposition vid
samma frameantal; siffrorna är inte ett kontrollerat PS4-benchmark.
Evidens: artifacts/video-013/ och artifacts/shader-probe-videoupload/.

Paket: Doom3-PS4-Playable-Alpha-0.13.pkg, verifierat med 15 uppackade filer.
SHA-256: 9884db9cb70997fe7ea9c22d704e2eba69df0bcc448fea14028f0c18e39c174f.
USB-status: artifacts/video-013/usb-delivery.json. 0.12 sparad som stabil bas.
Fysisk 0.13 återstår: intro, gameplay, PDA/skärmvideo, ljud och stabilitet.
Ändringarna är ocommittade.

---

# Aktuellt 2026-10-09 – 0.12 shader-minnesläcka

Fysisk 0.11 kraschade trots flyttad mixning. 0.09 och 0.11 har samma sista
shaderloggsekvens; det är en ledtråd, inte en symboliserad feladress.
Hittad konkret läcka i PSBC: temporär NIR frigjordes inte efter kompilering.
0.12 frigör den efter sista användningen, även vid ACO-fel. Läs
[SHADER-MEMORY-012.md](docs/SHADER-MEMORY-012.md).

Hostregression före/efter: 450 kompileringar, RSS-tillväxt efter uppvärmning
24 608 768 -> 65 536 byte. scripts/test-psbc-memory.sh passerar också.
Ljudkoden är hashidentisk med 0.09, ljud på; 0.11-kön är borttagen.
Vulkanpatchen är fortfarande committad 0.07, endast PSBC:s minnesstädning ändras.
600 introframes i emulator, ljudutmatning och rent avslut passerade.
Fysisk 0.12 återstår: läckfixen är verifierad, kraschfixen är inte det.
Paketvalidering passerade med 15 filer. Paketet är 38 862 848 byte.
SHA-256: 9acdcf24a0398bec75365904dceb23832563beaccec4e8ce67a88c7f45398a83.
USB-resultat: artifacts/memory-012/usb-delivery.json. Evidens i samma katalog.
Ändringarna är fortfarande ocommittade.

Användaren frågade även om fjärrinstallation/start kan byggas. Svar: ja;
Remote Package Installer har HTTP-API men kräver förgrundsapp. Automatisk
start/styrning behöver firmwarekompatibel integration och fysisk verifiering.
Inget fjärrverktyg har byggts eller installerats i denna tur.

---

# Aktuellt 2026-10-09 – 0.11 mixning på huvudtråden

På användarens önskemål behålls ljudet på och trådningen ändras.
`alcRenderSamplesSOFT` körs efter `common->Frame`, på samma tråd som ljudvärldens
uppdatering (`com_asyncSound 0`). AudioOut-tråden konsumerar bara färdig PCM.
En statisk SPSC-kö rymmer 24 x 512 frames (256 ms), separata dubbla
utmatningsbuffertar bevaras. Tom kö ger tystnad. OpenALs interna trådar kvarstår.
Se [AUDIO-THREAD-011.md](docs/AUDIO-THREAD-011.md).

Emulator: Nytt spel/intro 600 frames och direkt mars_city1 180 frames,
båda rent avslut och icke-tysta ljudblock. Detta är inte bevis på konsolfix.
Intro gav 3751 icke-tysta block; direktmap 563. Underruns inkluderar laddning
utan pump och testets fem sekunders fångstpaus, inte en ren gameplay-mätning.
Paketvalidering och 15 uppackade filkontroller passerade.
USB: `Doom3-PS4-Playable-Alpha-0.11.pkg`, 38 862 848 byte, full återläsning
matchar SHA-256 `5feb5acdaba6708e6b322a36f905c8f0e67720920d1b2b5704a6a03d6616dd26`.
Fsync klart, stickan monterad. Evidens: `artifacts/audio-thread-011/`.
Grafikpatch exakt committad 0.07; samma interna speldata; inga videoändringar.
Fysisk 0.11: användaren rapporterade laggigare ljud och fortsatt krasch.
FTP-logg sparad i artifacts/crash-011-ftp-20261009-103048/.
Ändringarna är ocommittade. 0.10 finns kvar som fungerande ljud-av-kontroll.

---

# Aktuellt 2026-10-09 – 0.10 ljud-av-kontroll

Fysisk 0.09 enligt användaren: bildkorruptionen försvann, kraschen återkom.
Ljud och extern dataväg är hypoteser; orsaken är inte fastställd.
0.09-loggar och faktisk konsolkraschdump med videor hämtades via FTP till
`artifacts/crash-009-ftp-20261009/`. Dumpen är inte symboliserad.

0.10 stänger av ljudet med `s_noSound 1`, behåller 0.07-grafiken och samma
interna data `/data/doom3-game/base`. Se [kontrolltestet](docs/AUDIO-OFF-010.md).
Emulatorn klarade mars_city1, 180 bildrutor och rent avslut; ljud-av bekräftat.
Paketvalidering passerade, samtliga 15 uppackade filer kontrollerade.
Paket: `dist/Doom3-PS4-Playable-Alpha-0.10.pkg`, 38 862 848 byte.
SHA-256: `593840b6297e4db94d988b2053c4664dd5fc3ece45fe1e046e7293b79920b02e`.
USB-leverans klar efter återanslutning i annan port: hela 0.10-kopian
återläst med matchande SHA-256 och fsync slutförd. Stickan lämnades monterad.
USB-resultat: `artifacts/audio-off-010/usb-delivery.json`.
Fysisk 0.10: användaren rapporterar lyckad start och fortsatt körning.
FTP-loggar i `artifacts/physical-010-ftp-20261009-101321/` bekräftar build
0.10, samma interna dataväg, laddad mars_city1 och gameplay med cinematic=0.
GPU-snapshot når frame=3120 med lyckad presentation. Användaren bekräftade därefter att 0.10 är långt förbi tidigare
kraschstället och inte längre kraschar i denna körning.
Det stärker ljudaktivering som utlösare men fastställer inte direkt felorsak.
Inga nya prestandaändringar ingår.
Källändringarna är fortsatt ocommittade.

---

# Aktuellt 2026-10-09 – 0.09 rollback efter fysisk 0.08-krasch

Användaren rapporterade bildkorruption och krasch med 0.08; introfilmen var
fortfarande långsam. Loggar hämtades direkt från PS4 FTP och sparades med
hashar i `artifacts/crash-008-ftp-20261009/`. Ingen coredump fanns. Sista
GPU-raden är PS-kompilering, inte ett fastställt felställe.

**0.09 återställer native Vulkan-patchen exakt till committad 0.07.**
OpenAL-arkivet och OpenAL-patchen är hashidentiska med 0.08; ljudet är kvar.
Läs [rollback-instruktionen](docs/ROLLBACK-009.md).
Paket: `dist/Doom3-PS4-Playable-Alpha-0.09.pkg`, 38 862 848 byte.
SHA-256: `9f150c956a636a4bc6546cd258491616ea9dc0e9ecc251fc7ff3daaa326685e1`.
Paketvalidering och uppackning/hashkontroll av 15 filer passerade.
Intern speldata `/data/doom3-game/base` används fortsatt; ingen ny dataöverföring.
USB-leveransens slutresultat finns i `artifacts/rollback-009/usb-delivery.json`.

Ny testväg: `CLIENT_TEST_NEW_GAME=1` kör startGame via samma väg som menyn,
inklusive `guis/intro.gui`. Tidigare `CLIENT_TEST_MAP` hoppade över detta GUI.
600 klientbildrutor och rent avslut verifierade i emulator, 3751 icke-tysta
ljudblock. Introsekvensen gav cirka 9–17 fps där; fysisk stabilitet återstår.
Mellanbilder visar introtext. Slutfångsten är svart och bevisar inte en korrekt
övergång till gameplay. Se `artifacts/rollback-009/intro/`.

Prestandaförslagen finns i [VIDEO-PERFORMANCE-NEXT.md](docs/VIDEO-PERFORMANCE-NEXT.md).
De är inte aktiva i 0.09. Börja med att jämföra exakt samma intro på fysisk PS4
för korruption/krasch. Rapportera både ljud och bild. FTP kan användas direkt;
lokal Diagnose 0.17 visar Doom-loggar men exportlistan saknar dem.

Källändringarna är fortsatt ocommittade. 0.08 och dess metadata är bevarade.

---

# Senaste distributionsval 2026-10-09 – intern PS4-data via FTP

Användaren valde därefter **litet paket på USB + speldata på intern PS4-disk**.
Paketet är `Doom3-PS4-Playable-Alpha-0.08.pkg` (38 862 848 byte), samma klient
som bundled 0.08 men utan PK4/markör. Installeras över 0.07 med samma TITLE_ID.
USB-kopian är återläst/hashverifierad och fsyncad; stickan lämnades monterad.
SHA-256: `f8d40420059a66d757076bd8a648c41085724c90a7c11f1ef7fc82911532abd4`.

FTP: `192.168.32.229:2121`; mål `/data/doom3-game/base/pak000.pk4` till
`pak004.pk4`. Slutstatus och per-filhashar finns i
`artifacts/audio-008-ftp-delivery.json` (complete=true; alla fem arkiv återlästa och SHA-256-verifierade).
Gamla konsolloggar sparades i `artifacts/ftp-008-before/` innan överföringen.
Den avbrutna stora 0.08-kopians .partial-fil på USB togs bort; 0.07 är kvar.
Nuvarande `artifacts/package/` avser det lilla paketet, 15 uppackade filer
verifierade. Bundled-metadata är bevarad i `artifacts/package-bundled-008/`.
Äldre bundled-rekommendation i dokumentet nedan är därmed ersatt.

---

# Aktuell status 2026-10-09 – 0.08 ljud/video

Repo: `/run/media/nichlas/Kingston1TB/Doom3-Ps4` (gamla `/home/nichlas/Doom3-Ps4`
är en symlänk). Läs [AUDIO-VIDEO-008.md](docs/AUDIO-VIDEO-008.md) före historiken nedan.

Användaren rapporterade att gameplay flyter okej på fysisk PS4, men både
berättelsescener och skärm-/PDA-videor hackar och ljud saknas. Den gamla klienten
hade tvingat `s_noSound 1`, OpenAL-stubbar och avstängd async-ljudtråd.

0.08 ersätter stubbarna med OpenAL Soft och en native AudioOut-utmatningstråd;
ljudvärlden uppdateras på huvudtråden. Ennivå-RGBA8-videotexturer använder nu
radvis GPU-kopiering i stället för pixelvisa adressberäkningar och små tilekopior.
GPU-väntan och PM4-minnets livslängd är bevarade.

Verifierat: 360 bildrutor Mars-intro och rent avslut i emulator; OpenAL Soft,
256 källor, 1438 icke-tysta block i sista utmatningsperioden. Användaren
bekräftade **hörbart ljud i emulatorn**. Ett femsekundersfönster efter laddning
gav 298 bildrutor/5008 ms, längsta bildruta 56 ms. Detta är inte en fysisk
PS4-prestandamätning eller ett kontrollerat 0.07/0.08-prestandatest.

Streaming-proben verifierade 921600 pixlar utan fel. Vulkan-, PM4- och
semafortester passerade. Se `artifacts/audio-008-final/` och
`artifacts/shader-probe-streaming/`. Kända intro-GUI-artefakter kvarstår.

**Nästa fysiska test:** ljud/synk i intro, vapen/miljöljud, PDA-video och
bildfrekvens i båda filmsorterna. Behåll saves/config. Ingen 0.08-konsolkörning
eller USB-leverans är ännu gjord. 0.07-paketet är bevarat i `dist/`.

Bygg: `scripts/build-ps4-client.sh`. OpenAL hämtas från checksummeverifierat
1.23.1-arkiv och patchas av `scripts/build-ps4-openal.sh`.
Paket: `python3 scripts/package-ps4-client.py --skip-build --bundle-data --game-data media/game`.
Verifieraren stöder nu explicit `--allow-bundled-data`; standardläget avvisar
fortfarande retaildata. Paketmetadata finns i `artifacts/package/`.
Äldre metadata sparades i `artifacts/package-before-008/`.

Slutpaket: `dist/Doom3-PS4-Bundled-Data-0.08.pkg`, 1595277312 byte.
SHA-256: `ec36c90f2a0febb514dc742338ba5498a09ab0a577776b4a40cb5f1a50dea2bc`.
PkgTool-validering och separat uppackning/hashkontroll av 21 filer passerade.
Även standardverifierarens avvisning utan bundled-opt-in är testad.

Alla ändringar i DUDE/native-arbetscheckout finns även i de beständiga patcharna.
Källändringarna är ännu inte committade. Retaildata och paket är Git-ignorerade.

---

# Handoff – Doom 3 till PS4

Uppdaterad: **2026-09-30**. Repo: `/home/nichlas/Doom3-Ps4`.
Kodcheckpoint vid denna genomgång: **`428e93b`**, `main`, i synk med
`origin/main`. Arbetskatalogen var ren innan handoffen skrevs.

## Börja här

**Spelet fungerar nu på fysisk PS4 med speldata i PKG**, enligt användarens
återkoppling 2026-09-30. Användaren anger att USB-stickan var för långsam för
filläsningen och att paketerad speldata löste problemet. Detta är det aktuella
hårdvaruresultatet; behandla inte USB-/arkivproblemet som en fortsatt blockerare
för det fungerande bundled-spåret.

**Fortsätt med paketerad speldata som huvudspår.** Senaste lokala kandidaten är
0.07. Användaren angav inte uttryckligen versionsnumret i den senaste
bekräftelsen, så knyt inte rapporten till en exakt binärhash utan ytterligare
uppgift. Rapporten bekräftar fungerande spel, inte ännu alla banor, långtidstest,
sparning/laddning eller ljud.

Historik: 0.05 gav `Couldn't reopen /doom3_usb0/DOOM3/base/pak000.pk4`.
0.06 följdes av rapporterad gameplay och problem med USB-läsning. 0.07 lade
till datavals-/reopen-audit och visade `/app0`-läsning i emulatorn. Den tidigare
hypotesen om `access()`/`fopen()` ska inte anges som fastställd grundorsak;
användarens senaste test pekar på USB-stickans läsprestanda.

Läs först [0.07-auditen](docs/BUNDLED-DATA-007.md), sedan
[0.06-bakgrunden](docs/BUNDLED-DATA-006.md) och
[installationsändringen 0.05](docs/INSTALL-005.md).

## Senaste lokala paket

| Fält | Värde |
| --- | --- |
| Paket | `dist/Doom3-PS4-Bundled-Data-0.07.pkg` |
| Storlek | 1 594 294 272 byte |
| TITLE_ID | `DOOM00001` |
| Version | `00.07` |
| CONTENT_ID | `IV0000-DOOM00001_00-DOOM3PS4MENUTEST` |
| Innehåll | Klient, ikon, loaderfiler, notices, fem retail-PK4 och bundled-markör |
| Staging | `build/client-package-bundled-007/` |
| Uppackning | `build/bundled-007-audit/` |

SHA-256, **omräknad och kontrollerad vid handoffen**:

```text
b987a0b8943e0193d4be96426d15de0a5f7f719b657cabd138606807dd79413e
```

`artifacts/package/manifest.json` avser just nu 0.07 och listar 19 filer.
Dess `source_revision` är `3fef53d…`, alltså revisionen före commit av
0.07-ändringarna. Använd paket-/ELF-hash och `build-info.json` för att identifiera
binären; anta inte att manifestets revisionsfält ensamt beskriver alla byggändringar.

Paketet med retaildata är lokalt och Git-ignorerat. Publicera inte paketet eller
spelfilerna. Källkod, patchar, scripts och egen ikon finns i Git.

### Två paketeringslägen

- Standard: **0.05**, litet paket med externa spelfiler. `--game-data` ensamt
  validerar filer, men inkluderar dem inte.
- `--bundle-data --game-data media/game`: **0.07**, lokal kandidat med arkiven i
  `/app0/base` och `/app0/doom3-bundled-data.txt`.

Äldre `DM3P00001` är inte aktuell paketidentitet. 0.05 bytte till `DOOM00001`
efter installationsproblem. Att bytet löste installationen är rapporterat;
den exakta orsaken till det tidigare felet är inte fastställd.

## Vad som är verifierat – och vad som återstår

| Område | Evidens och begränsning |
| --- | --- |
| Fysisk installation | Användaren bekräftade 0.05-installation och meny/video. |
| Fysisk gameplay | Användaren bekräftar 2026-09-30 att spelet fungerar med speldata i PKG; den långsamma USB-stickan anges som orsaken till tidigare läsproblem. Exakt version anges inte i senaste rapporten. |
| 0.07-emulator | `game/mars_city1` laddades; `PS4 GAME` tick 30/60/90 med health 100 finns i loggen. `/app0` används vid arkivåteröppning. Körningen var tidsbegränsad; hävda inte ett fullständigt speltest eller rent avslut. |
| Tidigare inputtest | 0.04: 360 bildrutor, rörelse/rotation/attack och release-kontroller, rent avslut. `artifacts/playable-package/`. |
| Kampanj | Tidigare `mars_city1`-test nådde 600 bildrutor; bildartefakter i öppningssekvensen finns dokumenterade. |
| Grafikprober | Flera pixeltester passerade, bland annat sampled presentation, djupkopior och offsetregioner. Det är komponenttester. |
| Ljud | Avstängt. |
| Sparning/laddning | Inte verifierad. Bevara befintliga saves/config. |
| Vulkan | Begränsad native Vulkan 1.1-väg, inte Vulkan 1.4-konformitet. |

0.07 ändrar dataval/loggning; renderer och ljudinställningar är oförändrade
jämfört med föregående kandidat.

## Nästa steg, i ordning

1. Bevara det fungerande paketet och använd bundled-data vid fortsatt lokal
   paketering. Scriptets standardläge är fortfarande extern data; ange därför
   `--bundle-data --game-data media/game` uttryckligen.
2. Testa längre spelpass och fler banor, sparning/laddning, handkontrollkänsla
   och kvarvarande grafikartefakter. Ljud är fortfarande avstängt.
3. Slutför stödet för bundled-paket i verifieringsscriptet, se nedan.
4. Anteckna installerad version/hash vid nästa relevanta test, så att
   hårdvaruresultatet kan knytas till exakt paket. Det hindrar inte fortsatt
   arbete utifrån användarens bekräftade fungerande spel.
5. Om läsfelet återkommer: spara loggar före omstart. Diagnose 0.12 visar
   ENGINE 1/6 och GPU 2/6. Kontrollera `DATA AUDIT`, filesystem-roots och de
   faktiska `reopen`-sökvägarna. Extern USB-läsning är ett separat framtida
   kompatibilitets-/prestandaspår, inte huvudspårets aktuella blockerare.

## Loggar och kodkarta

Consoleloggar:

```text
/data/doom3-client/dudelog.txt
/data/client-vulkan.log
```

Viktiga lokala filer:

- `patches/002-ps4-client.patch`: beständig DUDE-port; marker/dataval i
  `neo/sys/linux/main.cpp`, reopen-audit i `neo/framework/FileSystem.cpp`.
- `build/dude-client-source/`: ignorerat arbetscheckout för DUDE. Ändringar här
  måste återföras till patchen för att överleva en ren rebuild.
- `platform/ps4/storage.cpp`: extern datalagring och USB-mappning.
- `platform/ps4/input.inc`: ScePad till motorns tangent-/musinput.
- `platform/ps4/present.cpp`, `present_spirv.h`: sampled fullscreen-presentation.
- `platform/ps4/videoout_wsi.cpp`: privat VideoOut/WSI-brygga, 1280×720.
- `patches/ps4-native/`: beständiga ändringar för Vulkan/OpenGNM/PSBC.
- `build/native/`: ignorerade dependency-checkouts och byggprodukter.
- `assets/doom3-icon.svg`, `assets/icon0.png`: egen Mars-ikon; PNG är 512×512 RGB.
- `artifacts/bundled-007-emulator.log`: aktuell audit-/gameplay-telemetri.
- `artifacts/bundled-007-build.log`, `bundled-007-extract.log`: bygg-/uppackningsloggar.
- `artifacts/package/`: senaste manifest, build-info och paketvalidering. Mappen
  återanvänds mellan versioner; kontrollera version, hash och tidsstämpel.

### Viktig kvarvarande scriptskillnad

`verify-ps4-package.py` **avvisar fortfarande paket med retail-PK4** efter
filhashkontrollerna. Den generella verifieraren är alltså inte uppdaterad för
`--bundle-data`, trots att 0.07-dokumentationen redovisar separat kontroll av
alla 19 filer. Kör den inte blint och tolka PK4-policyfelet som filkorruption.
`artifacts/package/extraction-check.json` har dessutom äldre tidsstämpel än
0.07-manifestet och får inte användas som självständigt 0.07-bevis.

Lämplig liten uppföljning: låt verifieraren skilja explicit bundled-läge från
standardläge, behåll spärren mot oavsiktliga retailfiler i standardpaket och
kontrollera den exakta markören/filuppsättningen i bundled-paket.

## Bygg och reproduktion

Kända lokala verktyg:

```text
OpenOrbis: /opt/openorbis/OpenOrbis/PS4Toolchain
create-fself: /home/nichlas/ut99-orbis/build/create-fself-current
shadPS4: /home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage
```

DUDE-pinning: `3b6872fe278802496cc7a8f8209b847c68efed2a`.
Övriga pins finns i `scripts/build-native-vulkan.sh`. UT99 och ScummVM är
referensprojekt; ändra dem inte som bieffekt av Doom-arbete.

```sh
cd /home/nichlas/Doom3-Ps4
scripts/build-ps4-client.sh
python3 scripts/package-ps4-client.py --skip-build --bundle-data --game-data media/game
```

Det skriver över vissa aktuella staging-/manifestfiler. Spara önskad evidens
innan rebuild. `--skip-build` förutsätter att rätt klient redan är byggd.

För en ny begränsad emulatorrunda med den befintliga uppackade 0.07-kandidaten:

```sh
CLIENT_EBOOT="$PWD/build/bundled-007-audit/eboot.bin" \
CLIENT_USE_PACKAGED_DATA=1 \
CLIENT_TEST_MAP=game/mars_city1 CLIENT_TEST_FRAMES=360 CLIENT_TIMEOUT=600 \
CLIENT_PROFILE="$PWD/build/handoff-007-profile" \
CLIENT_ARTIFACTS="$PWD/artifacts/handoff-007" \
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-ps4-client.py
```

Detta är en föreslagen nästa körning, inte en körning utförd för handoffen.
Profilen måste sakna extern `doom3-game`-länk. Runnern skriver testmarkörer
bredvid sitt eboot; de hör inte hemma i konsolpaketet. Normalpaketet kör
kontinuerligt och har inte det ursprungliga menytestets 60-frame-avslut.

Riktade regressioner vid ändringar i berörd kod:

```sh
scripts/test-vulkan-features.sh
scripts/test-pm4-lifetime.sh
scripts/build-shader-probe.sh sampledpresent
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py sampledpresent
```

Bevara PM4-kommandobuffertarnas direkta GPU-minne. Den gamla heap-vägen
fungerade i emulatorn men var en fysisk hårdvarublockerare. Se
[Direct PM4](docs/DIRECT-PM4-HARDWARE.md). Återställ inte heller äldre clear- eller
DMA-presentationsvägar utan en specifik regression som motiverar det.

## USB och historiska dokument

Vid denna genomgång syntes `SCUMMVM_PS4` på `/dev/sdf1`, **utan mountpoint**.
Stickans aktuella filinnehåll och eventuell 0.07-leverans har inte lästs eller
verifierats i denna tur. Anta inte att 0.07 ligger där bara för att äldre
versioner levererats. Kontrollera enhet/mount på nytt före kopiering.

Vid leverans: behåll `.partial` under skrivning, flush/fsync, jämför hela
SHA-256 vid återläsning och avmontera säkert. Stickan har tidigare skrivit
mycket långsamt. Rör inte befintliga UT99-/ScummVM-paket eller saves.

README, PS4_PACKAGE och PLAYABLE-ALPHA innehåller äldre rubriker om
senaste version och fysisk verifiering. Läs dem som daterad historik där de
motsäger 0.07-auditen. Denna handoff och `BUNDLED-DATA-007.md` är startpunkterna
för fortsatt arbete. Handoff-turen har endast granskat lokalt material,
kontrollerat 0.07-paketets hash och skrivit dokumentation; inga nya emulator-
eller konsoltester har utförts.
