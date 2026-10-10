# Nästa videoexperiment efter 0.09

Status 2026-10-09: designunderlag, ingen av nedanstående optimeringar är
aktiverad i 0.09. Konsolens korruption/krasch måste först jämföras med rollback.

## Ny evidens

`CLIENT_TEST_NEW_GAME=1` kör samma `startGame`-väg som menyn. Den laddar
Mars-banan och växlar till `guis/intro.gui`; direkt `CLIENT_TEST_MAP` hoppade
över detta GUI. 0.09 körde 600 klientbildrutor och avslutade rent med 3751
icke-tysta ljudblock i sista ljudperioden. Mellanbilder visar introtexten;
slutfångsten är svart och bevisar inte en korrekt övergång till gameplay.

Femsekundersintervall i denna emulatorkörning gav cirka 9–17 fps. De sista
500 kommandobuffertarna hade median 461133 dwords, cirka 1.84 MB per buffert.
Detta är kommandovolym, inte bevis för var CPU/GPU-tiden går. Ingen kontrollerad
0.08/0.09-prestandajämförelse på fysisk PS4 finns ännu.

Båda introfilmernas RoQ-header anger 512x512, 30 fps. GUI:t innehåller två
videolager. Vid cinematic-bindning anropas UploadScratch även när avkodaren
inte producerat en ny bild. Samma globala måltextur delas av alla filmer.

## Första kandidat: cache av adressberäkningar, gammal layout

Behåll 0.07:s tiled GNM-texturer och generera exakt samma GPU-kopior som i
den fungerande referensvägen. Beräkna listan över sammanhängande käll-/målspann
en gång för en given layout, mip, region, texelstorlek och källpitch. Återanvänd
relativa offset/byteantal vid följande videouppladdningar; aldrig gamla
bufferadresser eller bilddata. Det undviker GpaComputeSurfaceCoord för varje
pixel i varje bildruta. Begränsa cacheminnet och invalidera vid layoutbyte.

Testkrav: jämför hela genererade kopieringslistan mot referensen för olika
storlekar, udda dimensioner, row pitch, offsetregioner och mipnivåer. Lägg till
GPU-pixeltester med två alternerande videor och flera bildrutor. Återanvänd
befintliga barriärer, staging-buffertarnas livslängd och GPU-väntan.

Vinsten kan vara begränsad: GPU:n får fortfarande samma många små kopior.
Mät därför avkodning, adressberäkning/kommandoinspelning och GPU-väntan separat
med aggregerade räknare, inte synkron filskrivning för varje pixel/draw.

## Nästa större kandidat: färre överföringar

Egen textur per cinematic och monotont avkodningsserienummer gör det möjligt
att återanvända en redan uppladdad videobild säkert. Pekarlikhet är otillräcklig;
RoQ återanvänder buffertar. Loop, rewind, omstart, storleksbyte och device-reset
måste invalidera cachningen. Detta får inte ändra filmens klocka eller ljudsynk.

Vid uppmätta 9–17 klient-fps kan varje 30-fps-video redan ha en ny bild vid
varje anrop. Att bara hoppa över identiska bilder behöver därför inte hjälpa
just denna sekvens, även om det minskar arbete vid fler anrop per videobild.

En senare möjlig väg till färre GPU-kommandon är att i en särskild, explicit
CPU-video-uploadväg lägga den avkodade bilden i den **gamla tiled-layouten**
i en staging-buffert och sedan göra större GPU-kopior. Det kräver separat
layout-/paddingverifiering och buffertlivslängdstest. Den generella Vulkan-
kopieringen får inte börja läsa källbufferten under kommandoinspelning:
källan kan vara GPU-skriven och måste i allmänhet läsas vid exekvering.
