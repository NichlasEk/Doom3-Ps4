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
