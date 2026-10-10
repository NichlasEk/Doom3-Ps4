# Fysisk IPC-diagnostik 0.10, 2026-10-09

Health bekräftade 0.10. BGFT laddning och symbolupplösning passerade.
Provet ipc-connect kördes exakt en gång. Funktionsprologskontroller passerade.
- client estimate: 0x1000
- IPMI Client::create: 0
- IPMI ClientImpl::connect: 0x80020001 (ORBIS_KERNEL_ERROR_EPERM)
- service outparam: 0x80020001
- disconnect: 0x80020003 (ESRCH, ingen anslutning etablerad)
- destroy: 0
- efterföljande health svarar, ingen krasch eller installation.

Källa för felkoder: lokal OpenOrbis include/orbis/_types/errors.h.
EPERM observeras på anslutningssteget, inte klientkonstruktion eller paketregistrering.
Det stöder behörighetsnekad anslutning. Avgör inte exakt credential/auth-ID/ACL
eller huruvida nekandet kommer från kerneltransporten eller tjänstens policy.
Det är en separat klient som återskapar native BGFT-konfigurationen, inte en
hook som observerar själva BGFT-initanropet; behåll denna skillnad i slutsatser.

Evidens:
- artifacts/remote-receiver/physical-010-20261009-103555.log
- artifacts/receiver-jobs/5d1262816bfc4b05b63affa76636ecca.json
- artifacts/remote-receiver/klog/20261009-103555-bd2d78e4.bin

Nästa arbete: fastställ runtime-identitet/IPC-accesskrav före credentialändring.
Tidigare SELF paid-byte ändrade inte observerat utfall; inte bevis för faktisk
runtime-auth-ID. Ingen ny paketvariant eller kerneländring gjord av detta test.
