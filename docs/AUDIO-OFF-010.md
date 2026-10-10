# 0.10 – ljud av, samma interna speldata

Fysisk 0.09: användaren rapporterar att bildkorruptionen försvann men att
kraschen återkom. Det stärker misstanken mot 0.08:s videoväg för korruptionen,
men fastställer inte orsaken till kraschen.

0.10 ändrar endast runtime-valet s_noSound från 0 till 1, utöver versionsmarkörer.
Grafikpatchen är fortfarande exakt committad 0.07. OpenAL är kvar länkad men
ljudsystemet är avstängt. Speldata läses fortsatt från /data/doom3-game/base.
Ingen ny videooptimering ingår i detta kontrolltest.

Emulator: mars_city1, 180 bildrutor, rent avslut; loggen bekräftar
Sound disabled with s_noSound 1. Detta bevisar inte fysisk stabilitet.
Testa samma start/intro/förflyttning som kraschade med 0.09. Om 0.10 också
kraschar behövs inte aktiv ljuduppspelning för felet. Om den klarar sig bör
jämförelsen upprepas innan ljudkoden pekas ut som direkt orsak.

0.09-loggar och sju kraschfiler inklusive videor och sorbisdmp är bevarade i
artifacts/crash-009-ftp-20261009/. Dumpen är ännu inte symboliserad.
Sista GPU-loggraden är shaderkompilering; den är inte en feladress.
