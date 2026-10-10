# Fysisk 0.16: LNC init OK, testtitel saknades; återinstallerad

2026-10-09 12:18-12:20 UTC. Health0.16. LNC analyzed entries resolved=0,
LNC initialize=0, scope restore=0. launch-test EN gång gav0x80940031.
Ingen körmarkör före/efter. Klog visar tidigare GameWillDelete(RMTS00002)
och borttagning av jobb535/536; aktuell launchApp(RMTS00002) nekas i
lnc_app_info query. FTP bekräftade /user/app/RMTS00002 frånvarande.
Detta provar inte fjärrstart med installerad app! Init är däremot verifierad.
Credentials separat avlästa; original auth/attributes/caps återställda.

Testappen återinstallerad via0.16 BGFT: task539(0x21b), register/start0,
status0/error0,6619136/6619136. Installerat app.pkg fullreadback matchar
85c6e21f7c9bb5ab57dc1dbda47135f2972279954a8559beaa6e2f027d211427.
Loggar: artifacts/remote-receiver/physical-016-lnc-init.log,
physical-016-launch.log, physical-016-after-reinstall.log,
physical-016-reinstall-readback.json.
Klog:20261009-121851-5d78aefb.bin och20261009-121948-3e96ac5c.bin.
Launchjournal1c89f1e991f24e92bb1531641137d68f, reinstall3657e13cb54d48259da3528d9bddf959.

Nästa kräver STÄNG och STARTA samma Receiver0.16 (ingen ny PKG), eftersom
vår egen launchTestAttempted-spärr tillåter ett försök/process. Bevara testappen.
Efter restart: kontrollera RMTS00002 finns FÖRE launch, markörbaseline,
health0.16, klog, lnc-init, restore, launch-test en gång, markör/klog/restore.
Ingen automatisk retry har skickats och ingen ny mottagarversion byggts.
