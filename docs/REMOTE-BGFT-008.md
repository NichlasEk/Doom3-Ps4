# BGFT utan AppInst – Receiver 0.8

Fysisk 0.7 visade att BGFT laddas, men AppInst nekas. Ingen installation utförd.
Undersökning: ps4-ipi använder AppInst enbart för titel-ID i install_pkg; själva
registreringen går via sceBgftServiceIntDownloadRegisterTaskByStorageEx, följd
av sceBgftServiceDownloadStartTask. Runtime-behörigheter kan ändå behövas:
referensappen använder libjbc, så 0.8:s fysiska framgång är inte säkerställd.

Källor:
- https://github.com/0x199/ps4-ipi/blob/main/Internal%20PKG%20Installer/installer.cpp
- https://github.com/flatz/ps4_stub_lib_maker_v2/blob/master/include/bgft.h

SDK-headerns packageSize/progressfält är uint32_t där referensen har 64-bitars
unsigned long. Lokal bgft.h använder uint64_t, statiska ABI-kontroller samt en
kompilerad jämförelse mot referensheadern passerade (param 104, storage 112,
progress 64 byte). Original-SDK ändras inte.

0.8 startar HTTP först, utan authinfo/kerneländringar. Samma ikon.
Fjärrkommandon med receiver-client.py:
1. probe --mode bgft (laddning + symboler)
2. probe --mode bgft-init (1 MiB heap, init separat, returkod i health)
3. bgft-register --manifest artifacts/package/manifest.json
   Full FTP-hashåterläsning före registrering. Mottagaren kontrollerar PKG-magic
   och titel-ID i headern; header-offset verifierad mot Doom013. Inte server-SHA.
4. bgft-start --task ID (separat mutation, inget automatiskt omförsök)
5. bgft-status --task ID (rå progress, installation_verified=false)

En uppgift per mottagarsession. Endast dess ID får startas/läsas. Ingen force,
uninstall eller start-all. Vid omstart förloras ID i mottagarens minne; journal
på PC och PS4-logg bevarar resultat. Registrera inte blint igen vid okänt utfall.
Install kan nekas eftersom Doom redan finns: ingen automatisk borttagning.

Verifierat: bygge, PKG-validering, ABI mot referens, sex hosttester,
emulatorns sex probe-lägen och health efter varje, auth/otillgängliga tjänster/
felaktiga task-ID/okänt prov. Emulator saknar BGFT: framgångsvägen är inte körd.
Fysisk BGFT-init, register/start/status återstår. 0.7 kan inte köra dessa nya API:er.
USB ej ansluten vid kontroll; 0.8 är endast lokalt byggd.
