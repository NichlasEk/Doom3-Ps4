# Fysisk0.17: installerad testtitel hittas; befintlig app blockerar

2026-10-09 14:03 UTC. Health0.17 attempts0/10. RMTS00002 finns viaFTP,
körmarkör saknas. LNC-init0 och restore0. Launch försök1 ger0x80940010.
Klog identifierar RMTS00002, categorygd, appVer00.02 och når
checkExistingApp där felet returneras. ShellUI öppnar LaunchConfirmScene /
ApplicationExitScene. Ingen körmarkör efter anrop. Detta är en annan spärr
än tidigare saknad app0x80940031. Receiver kör själv som BIG_APP.
Återställning verifierad i logg efter launch. Ingen blind retry skickad.
Användaren tillfrågad om synlig stäng-befintlig-app-dialog; inväntar svar.
Nästa: om OK valts, kontrollera markör och klog; hävda inte start utan bevis.
För helt obevakad start behöver livscykeln hantera att receiver upptar
foreground/BIG_APP-platsen; sannolikt separat bakgrundskomponent eller
systemets överlämning. Inte en fråga om fler identiska startförsök.

Evidens: artifacts/remote-receiver/physical-017-{init,launch}.log
artifacts/remote-receiver/klog/20261009-140323-8cb8af3c.bin
Launchjournal artifacts/receiver-jobs/d82157493ca841e8b93eb78a35b16312.json
