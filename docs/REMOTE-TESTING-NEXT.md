# Nästa arbete – fjärrinstallation, körning och styrning

Status 2026-10-09: första datorversionen finns i
[tools/ps4remote](../tools/ps4remote/README.md). FTP/logginsamling är fysiskt
verifierad. Installationsprotokoll och paketserver har lokala integrationstester.
Konsolmottagare, fjärrstart och styrning återstår. Konsol: 9.60 + Lapse via webb,
ännu ingen Remote Package Installer/Chiaki enligt användaren.
Användaren vill fortsätta med detta efter dokumenterad Doom 3 0.13-körning.

## Första kedjan

1. Identifiera konsolens firmware, jailbreak/HEN och tillgängliga tjänster.
   Känd FTP-adress är 192.168.32.229:2121; FTP ger filåtkomst, inte installation/start.
2. Servera ett exakt hashidentifierat testpaket på datorns LAN-adress.
   Installera via befintligt HTTP-API och följ installationsjobbet till klart.
   Remote Package Installer har `/api/install` och `/api/get_task_progress`;
   dess app behöver vara i förgrunden för att ta emot kommandon.
3. Verifiera rätt titel/version, starta via verifierad kompatibel styrväg.
   Blanda inte ihop paketnedladdning, installation och faktisk spelstart.
4. Samla versionsmärkta loggar, bild och eventuella kraschfiler per körning.
   Återanvänd Doom 3:s testmarkörer men ge fysisk körning eget run-id och timeout.

## Styrning och bild

Chiaki-ng/Remote Play är en kandidat för bildström och knapp-/spakinmatning.
Parning och kompatibilitet med denna konsol måste provas först; inga kontouppgifter
eller nycklar ska checkas in. Chiaki-ng har SDL GameController-integration.
SDL-bindningar ensamma transporterar inte kommandon till PS4 eller ger en bildström.

Ett litet lokalt verktygs-API för assistenten kan erbjuda capture_frame,
press_button(button,duration_ms), set_stick(x,y,duration_ms), release_all,
start_title och collect_logs. Begränsade hålltider, explicit knappsläpp vid
anslutningsfel och bildbekräftelse efter menyval behövs för reproducerbara tester.
Implementera ovanpå en testad transport, inte antagandet att en lokal SDL-event
automatiskt styr konsolen. Remote Play kan belasta konsolen; kör även kontroller
utan stream innan dess siffror används som prestandamätning.

Alternativ för egna spel: nätverksstyrd testinmatning i portens inputlager,
med run-id och bild-/loggexport. Kan styra Doom/UT99 men inte PS4-systemmenyn
eller paketinstalleraren. Håll verktyget fristående så flera portar kan användas.

Först verifieras installation och start separat, sedan knapptryck + bildåterkoppling,
sist automatiserad installera–starta–testa–samla-logg-kedja. Ingen ny funktion
ska beskrivas som fungerande förrän utfallet på fysisk PS4 är bekräftat.

## Primärkällor kontrollerade 2026-10-09

- Remote Package Installer API och förgrundskrav:
  https://github.com/flatz/ps4_remote_pkg_installer
- Chiaki-ng SDL GameController-byggstöd:
  https://github.com/streetpea/chiaki-ng/blob/main/CMakeLists.txt
