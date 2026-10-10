# 0.09 – återställd grafik, ljud kvar

2026-10-09. Installera det lilla `Doom3-PS4-Playable-Alpha-0.09.pkg` över 0.08.
Speldata finns redan på PS4 i `/data/doom3-game/base`; inga nya PK4 behövs.
Samma TITLE_ID `DOOM00001`. Behåll saves och config.

## Vad testet ska avgöra

Användaren rapporterade bildkorruption och krasch med 0.08 på fysisk PS4;
introfilmen var fortfarande lika långsam. Kraschloggar finns arkiverade i
`artifacts/crash-008-ftp-20261009/`. De identifierar 0.08 och intern speldata.
GPU-loggen slutar vid PS-kompilering men innehåller inget uttryckligt fel eller
stackspår. Den sista raden bevisar inte att kompilatorn är grundorsaken.

0.09 återställer hela native Vulkan-patchen till den committade 0.07-versionen:
0.08:s linjära texturlayout och dess radvisa snabbväg är borttagna.
Ljudbackend, OpenAL-patch och ljudinställningar från 0.08 behålls.
Detta är ett kontrollerat återställningstest, inte en bekräftad konsolfix.

Startmarkören är `BUILD 0.09 graphics=0.07 audio=0.08 video-experiment=off`.
En opt-in testmarkör låter emulatorn välja samma `startGame`-väg som menyn,
inklusive `guis/intro.gui`. Markören ingår inte i installationspaketet och
kräver dessutom ett uttryckligt ändligt test med frameLimit.

## Ny prestandakandidat – separat från 0.09

Inspektionen visar att intro-GUI:t har två RoQ-videolager, `introid` och
`introloop`, båda 512x512 och 30 fps. Den tidigare emulatorrundan med direkt
`map game/mars_city1` hoppade över just detta GUI och verifierade därför inte
hela introsekvensen. `PS4 PERF` mäter klientbildrutor, inte unika videobilder.
60 klient-fps bevisar inte att en 30-fps-film uppdateras korrekt.

Nuvarande Vulkan-väg skickar varje cinematic-bindning genom samma globala
`cinematicImage`, beräknar medelfärg och laddar upp bilden igen, även när
avkodaren lämnar samma videobild. Flera olika videor kan dela denna måltextur.

Nästa avgränsade kandidat:

1. Behåll den gamla, tiled GNM-layouten och befintliga GPU-barriärer/väntan.
2. Ge varje cinematic en egen GPU-textur med ägarstyrd livslängd.
3. Låt avkodaren ange ett monotont bildserienummer. Ladda upp endast när
   serienumret ändras; ta även hänsyn till rewind, loop, ny storlek och GPU-reset.
   Pekarlikhet räcker inte eftersom RoQ återanvänder sina bildbuffertar.
4. Mät separat avkodade/återanvända bilder, faktisk videoframåtrörelse,
   uppladdningar/byte och CPU-tid. Jämför samma tidsintervall av introfilmen.
5. Pixeltesta två alternerande videor, upprepade bilder, rewind/loop och
   texturbyte innan ett separat konsolpaket byggs. Behåll ljud/video-synk.

Detta minskar onödiga överföringar utan att ändra texturformatet. Vinsten är
ännu inte mätt; två lager som uppdateras vid varje klientbildruta kan annars
fortsätta orsaka kopieringar även när samma 30-fps-bild visas två gånger.
Kandidaten är inte implementerad eller aktiverad i 0.09.

## Konsoltest

Starta Nytt spel och låt samma introsekvens gå utan att hoppa över. Kontrollera
korruption, krasch och ljud. Anteckna om felet uppstår i text-/videointrot eller
efter övergången till den renderade Mars-scenen. Låt FTP vara på efter testet.
Loggarna läses direkt från `/data/doom3-client/dudelog.txt` och
`/data/client-vulkan.log`, så USB-export behövs inte för denna jämförelse.
