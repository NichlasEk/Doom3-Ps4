# Fjärrstart och första fysiska Doom-mätning, 2026-10-10

## Fjärrstart verifierad

Receiver0.24 PID217, worker218, ps4debug-NG1.3.2 på firmware9.60.
Worker LNC init0. Native async start av DOOM00001 returnerade0, ticket0.
Ny process221 identifierades som DOOM00001 /app0/eboot.bin.
Klog visar att RMTT00001 suspenderades under appbytet. Datorn återställde
workerns system-bit via separat debuggeranslutning och verifierade originaltuplen
0x3800000000000010 / 0x2000038000000000 / 0xff00.
Detta är fysisk verifiering av fjärrstart trots Receiver-suspension, inte bara en
accepterad biljett. Ingen ny PKG eller manuell bekräftelse behövdes i detta försök.

Journal: artifacts/ps4-probe/jobs/7bf08fce24b84331aba9906052dcd50b.json.
Klog: artifacts/ps4-probe/doom-launch-klog/20261010-080148-5002d881.bin.
Första förkontrollen avbröt före mutation: host-fingerprint hade33 i stället för32
bytes. Efter korrigering matchade nativekod. Ingen misslyckad appstart återförsöktes.

Hoståterställning klarar pausad worker, men inte garanterat förlorad nätkontakt
eller hostkrasch. Ingen konsolbaserad watchdog är byggd. En4KiB scratch-allokering
och RPC-stub finns kvar i worker218 tills processen avslutas.

## Mätning

Färsk logg identifierar BUILD0.13 graphics0.12 video cpu-tiled-staging audio0.09.
Kodfingerprints för main/CommonFrame/EndFrame matchade lokal dude.oelf.
Read-only sampling:147 observationer under29.810s,595 motorramar,19.960 FPS.
Detta är motorräknarens hastighet, inte en visuell felfrihetskontroll.
Scenen har inte identifierats av användaren; kalla inte detta gameplay- eller
introbenchmark. time_gameFrame/time_gameDraw/time_frontend/time_backend stod0.
De äldre räknarna kan därför inte ange delsystemens kostnad i den här mätningen.
com_frameTime ökade9917ms under29.810s hosttid; orsaken är inte utredd.

300 senaste kompletta presenteringscykler i färsk Vulkanlogg:

| CPU-väggtidsintervall | Median ms |
|---|---:|
| Mellan avslutade presentationer | 50.051 |
| Föregående presentation till färdig kommandobuffert | 23.601 |
| Färdig kommandobuffert till inlämningsmarkör | 0.609 |
| Inlämningsmarkör till presentation | 15.629 |
| Blockerande presentation | 10.152 |

Mellan två SubmitDone-returer är median15.529ms. Koden väntar på riktiga
GPU-completion-labels och gör blockerande vsync-flip. Intervallen inkluderar
loggning, schemaläggning, väntan och eventuell GPU-arbetstid; de är inte GPU-timers.
Första intervallet inkluderar även motorns pacing och får inte kallas ren CPU-rendering.
Medianerna summerar inte nödvändigtvis till medianen för hela ramen.

Detta prioriterar mätning av serialiserad completion/presentation och pacing.
Det bevisar inte att GPU:n är långsam eller att videoavkodningen är flaskhalsen.
Ta inte bort completion-väntan: den skyddar återanvändningen av kommandobuffertar.
Nästa profilering behöver samma identifierade scen plus separat timing för
avkodning, texture staging, command build, completion och flip; jämför ett lugnare
loggläge för att mäta påverkan av den täta breadcrumb-loggningen.

Evidens: artifacts/ps4-probe/doom221/{code-identity.json,timing-01.json,
vulkan-after-sample.log,present-analysis.json}. Loggarna är separata ögonblicksbilder,
inte exakt samtidiga. Ingen prestandafix har lagts in i Doom under detta försök.

## Verktyg

Nya probe/worker_scope.py och launch_doom.py implementerar begränsad fjärrstart.
probe/sample_doom.py läser timingräknare utan motor-RPC eller paus.
probe/analyze_present.py analyserar logggränser.15 hosttester passerar, inklusive
avvisad återanvänd process/credential, ändrad baslinje och ogiltig transition.

## Uppföljning: intro via fjärr-SDL och konkret klockfel

Användaren valde hackande intro/video. Verktyget doom_sdl.py installerade RPC
endast i Doom221 (stub0x4000, scratch0x8000), verifierade SDL_PushEvent-koden mot
lokal ELF och skickade vanliga tangent-/textevent för `testGUI guis/intro.gui`.
Alla15 SDL-event accepterades. Spelets logg visar kommandot; sessLocal.guiTest är
icke-null och objektet innehåller namnet guis/intro.gui. Detta verifierar GUI-testet,
inte fulla New Game-flödet med kartladdning. GUI-testet är fortfarande aktiverat.
Det körs på huvudtråden via normal input; inga osynkroniserade engine-kommandon
kördes på debuggerns RPC-tråd. SDL dokumenterar PushEvent som trådsäker:
https://wiki.libsdl.org/SDL2/SDL_PushEvent
Journal artifacts/ps4-probe/jobs/sdl-1791619733948766715.json.
Återanvänd inte doom221-sdl-state.json efter omstart/PID-återanvändning.

Ny30s-sampling:276 motorramar på29.978s =9.207 FPS.
Evidens timing-intro-01.json och intro-identity.json i doom221-katalogen.
Separat Vulkanlogg efter SDL-kommandot analyserades från408000ms loggtid,
efter övergången till introkommandobufferten på1626dwords:104 hela cykler.
Medelvärden:109.084ms totalt,95.118ms före färdig kommandobuffert,
0.192ms till submission,6.318ms till present,7.456ms i blockerande present.
Alltså ligger cirka87% före färdig kommandobuffert i denna introsekvens.
Det prioriterar avkodning/CPU-texturpackning/pacing före rena GPU-optimeringar.
Det skiljer fortfarande inte avkodning från tiled staging, memcpy, ljud och sleep.
Ingen synkroniseringsväntan får tas bort enbart på dessa siffror.

### Klockans ABI är fel i nuvarande motorbygge

Disassembly Sys_MillisecondsPrecise0x43b980 skickar clock_gettime(1,...).
Installerad OpenOrbis-musl include/time.h definierar CLOCK_MONOTONIC till1.
FreeBSD9 definierar1 som CLOCK_VIRTUAL och4 som CLOCK_MONOTONIC:
https://raw.githubusercontent.com/freebsd/freebsd-src/releng/9.0/sys/sys/time.h
Live RPC via samma clock_gettime-PLT0xc76070 gav status0 för både1 och4.
Clock1 värden187.073→189.761s; clock4 värden93595.429→93598.554s.
Frågorna var sekventiella; hostintervallet3.205s är inte exakt samma mätfönster
för båda klockorna. I tidigare lugnare scen ökade engine-tiden bara9.917s under
29.810s hosttid. Engine PERF sade senare300frames/5000ms (60FPS) medan host och
Vulkanlogg visade cirka20FPS. De gamla engine-FPS-siffrorna är alltså missvisande.

ABI-definition, maskinkod och liveprov pekar ut konkret felaktigt clock-ID,
inte bara en generell prestandahypotes. Nästa kodfix: välj PS4:s native monotona
klocka för både Posix_InitTime och Sys_MillisecondsPrecise, från processstart.
Byt inte bara klockan mitt i körningen: den sparade tidsbasen skulle bli fel.
Gör därefter om samma introprov och mät separat RoQ decode och video staging med
verifierad väggklocka. Klockfixen är ännu inte implementerad, byggd eller installerad;
högre FPS eller korrekt ljud/video-synk efter fixen är inte bevisad.
