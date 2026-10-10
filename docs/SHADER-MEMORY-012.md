# 0.12 – frigör temporärt shader-minne

Fysisk 0.11 kraschade fortfarande och ljudet blev laggigare enligt användaren.
FTP-loggar: artifacts/crash-011-ftp-20261009-103048/. Byggidentitet bekräftad.
Sista shaderloggen är VS 5568 byte, samma slutsekvens som 0.09. Detta är inte
en bevisad kraschadress. Inga coredumps fanns på FTP vid hämtningen.

Granskning hittade en faktisk läcka i psbc_compile_shader: temporär NIR med
ralloc-barn frigjordes inte efter buildshaderbinary eller vid ACO-fel.
psbc_shutdown hanterar bara referensräkning för GLSL-typer, inte NIR.
0.12 frigör NIR efter sista användningen. Output äger sina egna kopierade bytes.

Hostregression: 450 kompileringar av clear-vertex, RSS-differens efter de första
50: före 24 608 768 byte, efter 65 536 byte. Alla 450 utdata var identiska inom
respektive körning. Detta bekräftar läckan, inte orsaken till PS4-kraschen.
Se tests/psbc-memory.c och artifacts/memory-012/{before,after}.log.

Ljudkoden återställd exakt till 0.09:s mixning/AudioOut-worker; ljud på,
com_asyncSound 0. 0.11:s PCM-kö och pump är borttagna. Vulkanpatchen är fortsatt
0.07; endast PSBC:s temporära minneslivslängd ändras. Speldata kvar på intern disk.
Fysisk test: samma intro och övergång till gameplay. 0.10 är ljud-av-kontroll.
