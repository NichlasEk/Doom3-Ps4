#pragma once
#include <AL/alc.h>
namespace ps4 {
ALCdevice *openAudioDevice();
bool startAudioOutput(ALCdevice *device);
void stopAudioOutput();
}
