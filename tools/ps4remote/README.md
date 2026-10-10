## Aktuell status 2026-10-10

Installation av Doom 0.14, fjärrstart, SDL-styrning och skärmbildshämtning är nu
fysiskt verifierade på PS4 9.60. Se [probe-verktygen](probe/README.md),
[installationsresultatet](../../docs/CLOCK-014-PHYSICAL.md) och
[styrning och bild](../../docs/REMOTE-CONTROL-20261010.md).
Avsnitten nedan dokumenterar verktygens äldre utvecklingssteg och begränsningar.

# PS4 Remote Tools 0.1

Första körbara datorversionen. Endast Python 3.11+ och standardbiblioteket.
Ingen ny programvara behöver installeras för FTP-funktionerna. Aktuell konsol:
192.168.32.229, firmware 9.60, Lapse via webb enligt användaren.

Verifierat mot fysisk PS4: FTP-kontroll och logghämtning med byggidentitet 0.13.
Verifierat lokalt: HTTP HEAD/GET/Range, parallella läsningar, felhantering,
manifest/hashkontroll och installationsutbyte med en testmottagare.
Fysisk paketinstallation är ännu inte verifierad: mottagare saknas på konsolen.
Fjärrstart, bild och knappstyrning är inte implementerade i 0.1.

Kör från projektroten:

```sh
python3 tools/ps4remote/ps4remote.py doctor
python3 tools/ps4remote/ps4remote.py collect --expect-build 0.13
```

`collect` skapar en ny katalog under `artifacts/remote-runs/` med UTC-tid och
unikt id. Originalfiler, SHA-256, remote paths, byggidentitet, fel och lista över
kraschdumpkataloger sparas. Dumpfilerna själva hämtas inte i denna version.
Det är en loggögonblicksbild: rätt BUILD betyder inte att en ny körning har startats.
Loggar kan vara buffrade och hämtas sekventiellt. Fel/mismatch ger exitkod 2.
Standardprofilen hämtar Doom-loggar; Python-funktionen collect tar även en explicit
namn-till-fjärrsökväg-mappning för andra portar.

## Paketserver och installation

Paketet måste ha en manifestfil med package, bytes, sha256, title_id och valfri
version. Projektets `artifacts/package/manifest.json` fungerar direkt. Paketets
storlek, SHA-256 och PKG-magic kontrolleras innan det används.

```sh
python3 tools/ps4remote/ps4remote.py serve \
  --manifest artifacts/package/manifest.json --bind 192.168.32.88 --port 8089
```

Servern skriver en URL med paketets SHA-256 och lämnas igång i terminalen.
Den exponerar bara det verifierade paketet, inga kataloger eller andra filer.
GET/HEAD och enskilda byte ranges stöds. Ingen installation sker av `serve`.
Ändra inte paketet under överföringen; servern vägrar om dess metadata ändras.
Servern har ingen autentisering: bind till den lokala LAN-adressen, öppna inte mot internet.

När en kompatibel Remote Package Installer körs i förgrunden på PS4:

```sh
python3 tools/ps4remote/ps4remote.py install \
  --manifest artifacts/package/manifest.json --url 'URL-FRÅN-SERVE'
python3 tools/ps4remote/ps4remote.py status --task 42
```

`install` skickar ett faktiskt installationsanrop. Den validerar serverns
Content-Length/ETag mot manifestet och sparar ett jobbprotokoll under
`artifacts/remote-installs/`. `accepted` betyder att mottagaren gav ett task-id,
inte att installationen är färdig. `status` hämtar rå framdrift/felkod.
RPI:s hexadecimala fält stöds utan eval och utan att ändra stränginnehåll.

Låt servern vara igång tills konsolen har slutfört installationen. Nedladdade
bytes bevisar inte färdig installation, rätt installerad version eller start.
Version 0.1 tolkar därför inte okända progress-bitar som framgång. Vid timeout
eller förlorat svar sparas `unconfirmed`; verktyget skickar inte automatiskt igen.
Kontrollera konsolens installationskö innan ett nytt försök.

Alla kommandon stöder globala `--host`, `--ftp-port` och `--rpi-port` före
underkommandot. Inga lösenord/parningsnycklar eller dataarkiv checkas in.

## Tester

```sh
python3 -m unittest discover -s tools/ps4remote -v
```

Tester använder temporära syntetiska filer och lokala HTTP-servrar. Ingen
installation skickas till PS4 av testerna. Resultat från första livekontrollen:
`artifacts/remote-doctor.json`, `artifacts/remote-collect.json`.

## Nästa leverans

Välj/bygg en mottagare kompatibel med 9.60 + Lapse och verifiera faktisk
installation av ett litet testpaket. Därefter start/stopp och byggidentitet.
Bild + styrning via en testad Remote Play/Chiaki-transport eller en separat
agent i våra portar; SDL är endast inmatningsdelen. Se även
[planen](../../docs/REMOTE-TESTING-NEXT.md).

Protokollkälla: [flatz Remote Package Installer](https://github.com/flatz/ps4_remote_pkg_installer),
särskilt `server.c` för is_exists, install och get_task_progress.

## Egen mottagare 0.2 (ny)

`dist/PS4-Remote-Receiver-0.2.pkg`, titel-ID `RMTT00001`, är en separat
OpenOrbis-app. Den använder port **12801**, inte RPI:s 12800. Datorns äldre
`doctor` kontrollerar RPI; använd därför `receiver-client.py health` för denna app.
Ingen patchning av kernel eller automatiskt uninstall/force-overwrite ingår.

Installera mottagarens paket från USB en gång och öppna **PS4 Remote Receiver 0.2**.
Låt den ligga i förgrunden. Första versionen har ingen egen meny/rendering;
skärmen kan stanna på startbilden. Nätverkssvaret och loggen avgör om den kör.

```sh
python3 tools/ps4remote/receiver-client.py health
python3 tools/ps4remote/receiver-client.py exists --title DOOM00001
python3 tools/ps4remote/receiver-client.py install --manifest artifacts/package/manifest.json
python3 tools/ps4remote/receiver-client.py launch --title DOOM00001
```

`install` är ett faktiskt installationsförsök. Det kräver fungerande AppInst,
verifierar lokalt paket, laddar upp via FTP till en unik .partial, återläser
hela filen med SHA-256 och byter sedan namn till `/data/ps4remote/<sha>.pkg`.
En redan uppladdad fil återläses också innan återanvändning. Mottagaren kontrollerar
titel-ID via GetTitleIdFromPkg och anropar AppInstallPkg. SHA-256-verifieringen
sker i datorverktyget, inte inne i mottagarprocessen. Behåll de förberedda filerna
oförändrade mellan verifiering och installationsanrop.

Mottagaren har nyckelskydd för muterande kommandon och titeluppslag. Nyckeln
skapas lokalt vid byggning i `artifacts/remote-receiver/receiver.key` och finns
också i det privata paketet; nyckeln skrivs inte till loggar eller versionshanteras.
GET health är öppen och visar bara protokoll/version/init-resultat.

`launch` försöker sceLncUtilLaunchApp för en installerad titel. Plattformens
returkod sparas, men faktisk spelstart måste bekräftas med ny BUILD-logg/bild.
Start av ett annat spel kan suspendera mottagaren och bryta anslutningen; ett
förlorat svar får inte orsaka automatiskt nytt start-/installationsförsök.
Återöppna mottagaren manuellt när det behövs. Allt är ännu oprövat på fysisk 9.60.

Logg: `/data/ps4remote/receiver.log`. Datorjournaler: `artifacts/receiver-jobs/`.
AppInst/LncUtil laddas dynamiskt; frånvarande tjänster ger ett diagnostiskt fel.
Ingen knappstyrning eller bildström finns ännu.

Byggning och test:

```sh
python3 tools/ps4remote/build-receiver.py
xvfb-run -a -s '-screen 0 1280x720x24' python3 tools/ps4remote/test-receiver-emulator.py
```

Verifierat: native bygge, paketvalidering, fem uppackade runtimefiler,
emulatorns health-svar samt felaktig nyckel (403), saknad AppInst (503) och
ogiltig titel (400). Emulatorn saknar de proprietära systemmodulerna och kan
inte verifiera installation/start. Doom 0.13 har förberetts via FTP med
full hashåterläsning; ingen installation eller start har ännu begärts på PS4.


### Receiver 0.3

Åtgärdar laddningsförsöket från 0.2 med interna systemmoduler och sandbox-sökväg.
Söker symboler i laddade moduler, inklusive launch, och loggar upplösningen.
`health` visar `install_available` och `launch_available` separat.
Paket: `dist/PS4-Remote-Receiver-0.3.pkg`. Samma titel och lokala nyckel som 0.2.
Bygge/paket/emulator-HTTP verifierade; fysisk tjänsteladdning återstår.
0.2 svarade fysiskt på HTTP men kunde inte initiera AppInst/Lnc.

### Receiver 0.7: service probes

Starts HTTP before service loading. Run `python3 tools/ps4remote/receiver-client.py probe --mode resolve`
(or direct, sandbox, internal, bgft). Authentication is required. Modes are a fixed
allowlist; no arbitrary path or code execution. Each probe logs entry/return and
resolves available service symbols. BGFT then internal tests a new loading order.
An absent service is not a successful installation. A crashing probe may require
manual app restart. Never automatically retry an unconfirmed request.

### Receiver 0.8

BGFT kan nu initieras, registrera lokal PKG, starta eget jobb och rapportera
progress utan AppInst. Se ../../docs/REMOTE-BGFT-008.md för testordning och
begränsningar. Fysisk installationsfunktion ännu ej verifierad.

### Receiver 0.9

Nya probe-lägen: bgft-public-init, dep-system, dep-user, dep-netctl, dep-http,
dep-ssl, dep-np. Publik init har separat status och heap; den låser inte upp
intern registrering. Använd collect-klog.py samtidigt med konsolprov.
Initordning ändrar processtillstånd; jämför nystartade kontroller innan slutsats.

### Receiver 0.14

BGFT-anrop använder nu tillfällig systembit (ucred+0x60 bit62), med känd
9.60-baseline, readback och återställningskontroll före HTTP-svar. Detta gäller
init, register, start och progress. Understöds av fysisk IPC-kontroll i 0.13;
verklig BGFT-init och installation i 0.14 återstår. Ingen full libjbc-profil.
Normal retur och C++ exception återställer; processkrasch kan inte köra cleanup.

Lokal guard-test:
`c++ -std=c++17 -Wall -Wextra -Werror tools/ps4remote/tests/system-scope.cpp -o /tmp/receiver-system-scope-test && /tmp/receiver-system-scope-test`

Efter installation/start: health, probe --mode bgft, probe --mode bgft-init.
Kontrollera receiver.log (native retur + "BGFT system scope restored") innan
registrering. Publik och intern init använder samma native objekt på 9.60;
kör inte båda efter lyckad init. Jobbregistrering och start är separata anrop.

### Receiver 0.16: begränsat starttest

Efter uppdatering: `receiver-client.py probe --mode lnc-init`, kontrollera
receiver.log och lnc_init=0, därefter `receiver-client.py launch-test`.
Endast installerade RMTS00002 tillåts. Ett försök/session, ingen automatisk
retry. Verifiera körning genom ny testapplogg via FTP. Se
../../docs/REMOTE-LAUNCH-016.md för 9.60-analys och suspensionsbegränsning.

### Receiver 0.17

Starttestet tillåter nu tio anrop per process. Health visar `launch_attempts`
och `launch_limit`; startsvar och logg visar försöksnummer. Endast RMTS00002.
Klienten gör inga automatiska omförsök; kontrollera körlogg vid oklart resultat.
Detta ersätter engångsspärren i0.16.

### Receiver0.18: miniapp-försök

Paketkategorin är nu `gde`. Fysisk miniapp- och bakgrundsdrift är ännu inte
verifierad. `probe --mode app-slots` läser big/mini-app-ID till logg och health.
Starttestet är fortsatt begränsat tillRMTS00002 och tio försök.
Se ../../docs/REMOTE-MINIAPP-018.md för kontrollordning och källjämförelse.
