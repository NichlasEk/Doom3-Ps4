# BGFT 9.60: analys av bibliotek från användarens PS4

FTP GoldHEN v2.2 levererade /system/common/lib/libSceBgft.sprx som ELF,
89 144 byte. SHA256: 0a381049b8a5d12a429faf547cca12c8026b9309cf44c18c7bca4d245642fa2f.
Binär, disassembly och reproduktionsskript sparade endast i ignorerade artifacts/
remote-receiver/console-modules. Ingen konsolfilsändring, ingen kodpatchning.

Exportnamn matchade via NID-hash mot lokala OpenOrbis-stubbar. ELF:s SCE
dynlibdata/symboler/relokeringar lästes från programheaders, inte sektioner.
NID-algoritm kontrollerad mot OpenOrbis/create-fself OELFGenDynlibData.go.

Modulrelativa adresser:
- sceBgftServiceInit: 0x200 -> gemensam funktion 0xc20.
- sceBgftServiceIntInit: 0x370 -> samma funktion 0xc20.
Båda använder samma objekt 0x10060. Publik/intern-provet i 0.9 var alltså
inte två oberoende implementationsvägar på denna firmware.
- 0xc90..0xcb3: parameterkontroll; fel ger 0x80990004.
  Heap måste finnas, minst 0x100000 byte och högsta 32 bitarna av storlek noll.
- 0xcd8/0xd40 -> 0xd9a: allokeringsfel ger 0x80990011.
- 0xd51: anrop via PLT 0xd8, GOT 0xc058. Symbolen identifierad som
  IPMI::Client::create(Client**, Config const*, void*, void*).
- Negativt resultat från create -> 0xdad -> 0x80990050.
- 0xd75: virtuellt anrop på klientobjektet, vtable+0x10.
  Negativt anropsresultat eller negativt utparametervärde -> samma 0x80990050.
  Exakt namn på virtuella funktionen ännu ej bekräftat; sannolikt anslutningssteg.
- 0xda0 anropar städning vid fel.

Slutsats med begränsning: Den observerade 0x80990050 motsvarar i den här
initfunktionen klientkonstruktion eller efterföljande IPC-steg, inte dess
parameter-/heap-allokeringsfel. Exakt gren och underliggande returkod är
fortfarande okända; detta bevisar inte vilket behörighetskrav som saknas.
Nästa prov bör observera underliggande create-/virtuell returkod, inte göra
fler heap-/modul-/publik-vs-intern-gissningar. Ingen ny PKG byggd.

## 0.10: separat rå IPC-klient

libSceIpmi från samma konsol hämtad som ELF. Exporternas NID matchade mot
SDK-stubbar: Config ctor 0x11b0, estimate 0x1230, create 0x1300,
ClientImpl connect 0x6c0, disconnect 0x7c0, destroy 0x630.
Config ctor skriver t.o.m. 0x157. Native BGFT använder SceBgft som servernamn,
0 på offset 0x10, 1 på 0x1c och 0x8400 i båda 64-bitarsfälten 0x28/0x30.
0.10 återger den konfigurationen via native ctor i separat objekt och loggar
create/connect/service-returer före BGFT:s felöversättning. Inget metodanrop
för installation utförs. Egen klient disconnect/destroy efter anslutningsförsök.

Provet kräver samtliga sex symboler och matchande första 16 kodbyte för varje
analyserad export. Det är ett kompatibilitetsskydd, inte en fullständig verifiering
av biblioteket. Estimate måste ge 0x1000. Högst ett försök per process, ingen
automatisk repetition. Ingen ändring av systemkod, kernel eller credentials.
Config, klient och heap är separata från BGFT:s interna klient.

Fysisk test: efter start 0.10, börja klog-capture, probe --mode bgft (resolver),
probe --mode ipc-connect och läs FTP-loggen. Nya provet kan fortfarande fallera
eller krascha; emulatorn kan bara verifiera att saknade symboler stoppar provet.
