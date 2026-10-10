# 0.13 – explicit tiled videouppladdning

Fysisk 0.12 enligt användaren: fungerar felfritt med ljud, video hackar.
0.13 behåller ljudet och NIR-minnesfixen från 0.12.

Endast UpdateTexture2D, som används för cinematic-frames, använder den nya
vägen. CPU-ägda RGBA8-pixlar packas i måltexturens befintliga tiled layout med
samma GpaSurfaceContext och koordinatfunktion som referensuppladdningen.
Padding nollställs. Packad bild kopieras in i befintlig staging-ring och
överförs sedan till GPU-texturen i stora DMA-block. Format/layout ändras inte.
Den generella vkCmdCopyBufferToImage läser fortfarande staging först på GPU:n.
Alla barriärer, flight-fences och staging-livslängder behålls.

Avgränsning: 2D RGBA8 UNORM, en mip, ett lager, inga bildflaggor eller RT,
max 1024x1024 och 16 MiB backing. Annat fall faller tillbaka till gamla vägen.
Minnes-, storleks- och koordinatgränser kontrolleras. Ingen cache eller nya
GPU-allokeringar införs; temporär CPU-vector frigörs efter staging-kopian.

GPU-regression videoupload testar upprepade ändringar av en 513x509-textur
(udda dimensioner/padding), sampling av alla texlar och hela 1280x720-bilden.
Introtestet använder riktiga 512x512-filmer. Emulatorhastighet är inte fysisk
prestanda. Testa samma intro på PS4, sedan PDA/skärmar, utan korruption/krasch.
