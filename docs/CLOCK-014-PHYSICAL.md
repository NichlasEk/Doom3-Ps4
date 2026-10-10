# Doom 0.14: native monoton klocka, fysisk fjärrinstallation och test

2026-10-10. Användaren bad bygga nytt paket och prova på PS4:an.

## Ändring och paket

`patches/002-ps4-client.patch` väljer native Orbis clock-ID4 för motorns
`D3_GETTIME_CLOCK` under `__PS4__`. Både Posix_InitTime och
Sys_MillisecondsPrecise använder samma tidsbas från processstart. Andra plattformar
behåller tidigare val. Ingen global SDK-header eller klocka i en redan körande
process patchades. Byggmarkör och paketerare är uppdaterade till0.14.

- Paket: `dist/Doom3-PS4-Playable-Alpha-0.14.pkg`
- Titel: DOOM00001, SFO00.14, externa data `/data/doom3-game`.
- Storlek:38862848byte.
- SHA256:`af7aad6b183a13370e756ccfefc688dbf744ff43460db588cb7b64cd436ade95`.
- Fullt runtimebygge, PKG-validering och extraktion/15filhashar passerade.
- Disassembly visar `mov edi,4` i Sys_MillisecondsPrecise. Samma kodbyte verifierades
  i körande process224 på fysisk konsol.
- Native Vulkan/PSBC-arkivhashar och OpenAL-källpatch är oförändrade mot0.13-manifestet.
  OpenAL-arkivet byggdes om efter Ninjas `premature end of file; recovering` och har
  annan hash. Paketparet är alltså inte bitidentiskt förutom klockkoden. Ingen
  avsiktlig ljudändring gjordes. Hävda inte ett strikt isolerat binärt A/B-test.

0.13-paketet finns kvar. Dess installerade fullhash matchade lokal återgångskopia
före bytet. Äldre oelf/eboot finns frysta i `artifacts/clock-014/baseline/`.

## Fjärrkedjan fungerade

1. Befintligt Doom221 avslutades genom vanlig SDL-konsolinput `quit`; processen försvann.
2. Receiver217/worker218 förblev suspenderade även efter Doom-exit.
3. Debuggern kör redan i ShellCore. Efter tidigare verifierad egen-worker-RPC lades
   nu en separat RPC-tråd och4KiB scratch i ShellCore42. Kernel-getpid och UserService/
   SystemService-funktioner identifierades via minneskartor och kodfingerprints.
4. Native LNC init0 och start av RMTT00001 gav submission0/ticket0. Båda HTTP-health
   började svara med samma PID217/218. Inga ShellCore-kodpatchar eller credentials
   ändrades. ShellCore-RPC-sessionen ligger kvar tills processen/konsolen avslutas.
5.0.14 laddades upp till `/data/ps4remote/<sha256>.pkg`; full återläsning matchade.
6. BGFT-register gav0x80990088: samma grundapp redan installerad. Ingen task skapades.
7. Tio konfig-/savefiler,10328522byte, säkerhetskopierades från `/data/doom3-client/`.
   Installerat0.13-paket hashverifierades mot återgångskopian. Externa spelarkiv ingår
   inte i paketet. Ingen userdata-delete-funktion användes.
8. AppInstUtil kunde inte laddas i workern (0x80020008), men native laddning i
   ShellCore lyckades. Exporter identifierades via NID och jämfördes med den lokalt
   extraherade konsolmodulens kod. Initialize0 och AppUnInstall(DOOM00001)0.
9. BGFT registrerade sedan task565, start0, error0 och38862848/38862848byte.
   HELA `/user/app/DOOM00001/app.pkg` lästes tillbaka och matchade kandidaten.
   Alla tio säkerhetskopierade filer fanns kvar med oförändrade hashvärden före start.
10. Worker-RPC startade Doom igen: submission0/ticket1, ny PID224. Workerns systembit
    återställdes och lästes tillbaka via separat debuggeranslutning trots suspension.

Ingen manuell konsolåtgärd, USB-flytt eller firmwareändring behövdes i denna kedja.
AppInstUtil-flödet följde explicit begärd ersättning av vår egen Doom-installation;
verktyget exponerar inte ett godtyckligt avinstallationsmål.

Helper-script `resume_receiver.py` och `replace_doom_preflight.py` är sessionsbundna
försöksverktyg, inte generella CLI:er: hårdkodade verifierade PID och journalspärrar.
Återkör dem inte efter omstart eller okänt utfall. Kernel- och modulkod måste
återidentifieras. Hostbaserad credential-återställning är fortfarande ingen
nätfelsoberoende konsolwatchdog.

## Fysisk mätning

0.14 identifierades genom färsk BUILD-markör, paketets fullhash och levande kodbytes.
Via SDL skickades samma `testGUI guis/intro.gui` som i0.13-provet. Det är intro-GUI,
inte hela New Game med kartladdning, gameplay och verifierad ljudupplevelse.

| Mätning |0.13|0.14|
|---|---:|---:|
| Hosttid i introprov |29.978s|29.953s|
| Motorramar |276|267|
| FPS mot hosttid |9.207|8.914|
| Framåtgång i motortid |26.017s|29.983s|

Klockkorrektionen är verifierad: motortid följer verklig tid. **Ingen FPS-vinst i
intro är belagd.** Annan videofas per väggtidsfönster och ombyggt ljudarkiv gör små
skillnader olämpliga som kausalt prestandabevis. Äldre engine-PERF använde CPU-klockan
och ska inte jämföras rakt av med nya FPS-tal.

Efteråt stängdes GUI-testet genom `testGUI`; guiTest=null verifierades i minnet.
Normal varm meny:449frames/14.987s =29.960FPS, motortid ökade14.983s.
Doom224 är kvar igång i menyn. Inga observerade processavslut under det fullföljda
intro- och menyprovet. Visuell felfrihet, hörbar ljudsynk och lång gameplay är inte
bekräftade av dessa fjärrmätningar.

Första korta menysamplingen fick en debugger-read-timeout efter32observationer;
den är markerad incomplete och används inte som färdigt benchmark. Debuggern och
spelet fortsatte svara. Senare kompletta prov passerade. Inget okänt muterande
anrop återförsöktes. `condump clock014-intro.txt` gav usage (ingen dump skapades);
engine-/Vulkan-loggarna användes som evidens i stället.

Nästa prestandaarbete: separat native-väggtid för RoQ decode, CPU tiled staging,
kopiering och pacing.0.13-provet placerade cirka95 av109ms före färdig
kommandobuffert; den kostnaden försvann inte genom rätt klocka. Behåll GPU:s
completion-/resurslivstidskontroller.

## Evidens

Allt under `artifacts/clock-014/` är lokalt/ignorerat:

- `package-manifest.json`, `package-verify.log`, `clock-disassembly.txt`.
- `console-backup/manifest.json`: save/confighashar och verifierad återgång.
- `installation-verified.json`: installerad pakethash och bevarade filer.
- `remove-old-package.json`, `launch.json`, `live-verification.json`.
- `sdl-intro.json`, `intro-timing.json`, `intro-summary.json`.
- `menu-warm-timing.json`, `menu-warm-summary.json`.
- `boot-vulkan.log`, `vulkan-during-test.log`, `engine-after-tests.log`, `klog/`.

Receiverjournaler: register ca85cf85e084477f936c1ccffea98511,
start65972b750aca40508e05d7875a6f6c4c. ShellCore-resumejournal:
`artifacts/ps4-probe/jobs/resume-1791620671834135910.json`.

15 probe-/scope-hosttester passerar; ändrade Pythonverktyg syntaxkontrollerades.
Klientpatchens reverse-check mot faktisk byggkälla passerar. Arbetskopians tidigare
orelaterade ändringar är kvar; ingen commit eller push gjordes.
