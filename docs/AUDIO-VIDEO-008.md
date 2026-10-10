# Doom 3 PS4 0.08 – ljud och videouppladdning

2026-10-09. Lokal testkandidat med samma TITLE_ID DOOM00001 som 0.07.
Använd bundled-data-paketet; de fem egna retailarkiven ska förbli lokala.

## Ändringar

- Ersätter tomma OpenAL-stubbar och tvingat `s_noSound 1` med OpenAL Soft
  1.23.1, loopback-mixning och native SceAudioOut, 48 kHz stereo float.
  Ljudvärlden uppdateras på huvudtråden (`com_asyncSound 0`); en separat
  utmatningstråd skickar 512 sampel åt gången och behåller två buffertar.
  Start-/utmatningsfel loggas; utmatning stoppas och tråden joinas före stängning.
- OpenALs PS4-semafor använder pthread-mutex/villkorsvariabel. Den tidigare
  POSIX-semaforvägen anropade `ksem_timedwait`, som den lokala emulatorn stubbar,
  vilket gav en snurrande händelsetråd. TLS-wrapper och destructorstöd behövs
  för OpenOrbis. OpenAL gör fortfarande riktig spatial mixning, resampling och EFX.
- Ennivå-RGBA8-texturer för sampling/uppladdning får linjärt alignad GNM-layout.
  Återkommande RoQ-uppladdningar kopieras radvis med kontroller av padding,
  koordinater och minnesgränser. Kopiorna utförs fortfarande av GPU:n;
  synkronisering och PM4-minnets livslängd är bevarade. Mipkedjor, cubemaps och
  render targets behåller tidigare layout.
- `PS4 PERF` rapporterar antal bildrutor/tidsfönster och längsta bildruta.
  `PS4 GAME` anger `cinematic=0/1` för att kunna skilja berättelsescener från spel.

Den direkta videooptimeringen gäller texturuppladdning, även videoytor inne i
berättelsescener. Den är inte bevis för att alla typer av berättelsescener är
snabba på fysisk PS4. Emulatortiming är inte konsoltiming.

## Verifiering

- PS4-klient och OpenAL byggda och konverterade till FSELF.
- Vulkan-funktionstester och PM4-livslängdstest passerar under ASan/UBSan.
- Semafortest: tom semafor blockerar; 4000 signaler/fyra väntande trådar,
  korrekt sluträkning och inga tappade väckningar.
- Ny streaming-prob: 3x3-textur med padding, upprepade GPU-uppladdningar och
  sampling. 921600/921600 pixlar korrekta, emulator exit 0.
- Mars-introt når ljudutgång med icke-tysta block. Användaren bekräftade
  hörbart ljud i emulatorn 2026-10-09.
- Fysisk installation, hörbart ljud/synk och filmhastighet i 0.08 återstår.
  Kända bildartefakter i intro-GUI är inte förklarade eller lösta av detta arbete.

## Reproduktion

```sh
scripts/build-ps4-client.sh
scripts/test-openal-semaphore.sh
scripts/test-vulkan-features.sh
scripts/test-pm4-lifetime.sh
scripts/build-shader-probe.sh streaming
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py streaming
python3 scripts/package-ps4-client.py --skip-build --bundle-data --game-data media/game
python3 scripts/verify-ps4-package.py --allow-bundled-data
```

OpenAL-källan hämtas från upstream-taggen 1.23.1 och arkivets SHA-256 verifieras
av byggscriptet. Anpassningarna finns i `patches/003-openal-ps4.patch`.
OpenAL Soft är LGPL; licensen inkluderas i paketets notices. TLS-destructorstödet
är återanvänt från det lokala UT99/OpenOrbis-projektets platform/ps4-kod.
Byggartefakter, patchhashar och OpenAL-arkivhash finns i build-info.json.

## Nästa konsoltest

Installera 0.08 över samma applikation, utan att radera saves/config.
Kontrollera menyljud, Mars-intro/dialog, vapen och miljöljud samt PDA-video.
Jämför intro och PDA med 0.07. Vid fel: behåll `/data/doom3-client/dudelog.txt`
och `/data/client-vulkan.log`, och ange scenen samt installerad version.
