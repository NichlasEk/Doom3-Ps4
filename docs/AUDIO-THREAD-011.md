# 0.11 – mixning på huvudtråden

Användaren bekräftar att 0.10 utan ljud klarar sig långt förbi kraschstället.
0.11 återaktiverar ljudet och flyttar alcRenderSamplesSOFT till huvudtråden,
efter common->Frame. com_asyncSound är fortsatt 0. AudioOut-tråden kopierar
endast färdiga PCM-block till egna dubbla utmatningsbuffertar och skickar dem.
Den gör inga OpenAL-anrop och läser inte ljudvärlden.

En statisk SPSC-kö med release/acquire skyddar 24 block à 512 stereoframes.
Kön rymmer 256 ms vid 48 kHz; utmatningsbuffertarna är också statiska.
Tom kö ger tystnad, inte väntan på speltråden. Stop/join sker före återställning.
Mixarbetet per pump är begränsat till antalet lediga block vid inträdet.
OpenALs egna interna hjälptrådar finns kvar. Detta isolerar vår utmatningstråd,
men bevisar inte någon specifik rotorsak till 0.09-kraschen.

Grafik är fortsatt committad 0.07, data /data/doom3-game/base. Ingen videoändring.
Testa samma intro-till-gameplay som tidigare samt dialog, rörelse och PDA-ljud.
Detta är ett stabilitetsexperiment: kölatens och ljudavbrott vid långa frames
är möjliga. Emulatorresultat och USB-leverans sparas i artifacts/audio-thread-011/.
