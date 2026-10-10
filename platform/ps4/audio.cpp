// OpenAL Soft supplies spatialization, resampling, streaming and EFX. The PS4
// owns the output clock: each blocking AudioOut submission advances 512 frames.
#include "audio.h"
#include <AL/alext.h>
#include <orbis/AudioOut.h>
#include <pthread.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace {
constexpr int frames = 512;
std::atomic<bool> stopping{false};
pthread_t worker;
bool running = false;
int port = -1;
ALCdevice *outputDevice = nullptr;
LPALCRENDERSAMPLESSOFT renderSamples = nullptr;
uint64_t submitted = 0, audible = 0;

void *outputLoop(void *) {
    // Retain both blocks until the native device has consumed them.
    alignas(64) float buffers[2][frames * 2] = {};
    unsigned next = 0;
    while (!stopping.load()) {
        float *pcm = buffers[next];
        renderSamples(outputDevice, pcm, frames);
        bool nonzero = false;
        for (int i = 0; i < frames * 2; ++i) {
            if (!std::isfinite(pcm[i])) pcm[i] = 0;
            if (pcm[i] > 1) pcm[i] = 1;
            if (pcm[i] < -1) pcm[i] = -1;
            nonzero |= std::fabs(pcm[i]) > 0.0001f;
        }
        const int result = sceAudioOutOutput(port, pcm);
        if (result < 0) {
            fprintf(stderr, "PS4 AUDIO output failed 0x%08x\n", unsigned(result));
            break;
        }
        ++submitted;
        if (nonzero && ++audible == 1)
            fprintf(stderr, "PS4 AUDIO first non-silent block submitted\n");
        next ^= 1;
    }
    sceAudioOutOutput(port, nullptr);
    return nullptr;
}
}
namespace ps4 {
ALCdevice *openAudioDevice() {
    auto openLoopback = reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(
        alcGetProcAddress(nullptr, "alcLoopbackOpenDeviceSOFT"));
    renderSamples = reinterpret_cast<LPALCRENDERSAMPLESSOFT>(
        alcGetProcAddress(nullptr, "alcRenderSamplesSOFT"));
    return openLoopback && renderSamples ? openLoopback(nullptr) : nullptr;
}
bool startAudioOutput(ALCdevice *device) {
    if (running) return outputDevice == device;
    int result = sceAudioOutInit();
    if (result < 0 && uint32_t(result) != ORBIS_AUDIO_OUT_ERROR_ALREADY_INIT) {
        fprintf(stderr, "PS4 AUDIO init failed 0x%08x\n", unsigned(result));
        return false;
    }
    port = sceAudioOutOpen(0xff, ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, frames,
                          48000, ORBIS_AUDIO_OUT_PARAM_FORMAT_FLOAT_STEREO);
    if (port <= 0) {
        fprintf(stderr, "PS4 AUDIO open failed 0x%08x\n", unsigned(port));
        return false;
    }
    outputDevice = device;
    stopping = false;
    submitted = audible = 0;
    result = pthread_create(&worker, nullptr, outputLoop, nullptr);
    if (result) {
        fprintf(stderr, "PS4 AUDIO thread failed %d\n", result);
        sceAudioOutClose(port);
        port = -1;
        outputDevice = nullptr;
        return false;
    }
    running = true;
    fprintf(stderr, "PS4 AUDIO started: OpenAL Soft / 48000 Hz stereo / 512 frames\n");
    return true;
}
void stopAudioOutput() {
    if (!running) return;
    stopping = true;
    pthread_join(worker, nullptr);
    sceAudioOutClose(port);
    fprintf(stderr, "PS4 AUDIO stopped: submitted=%llu nonSilent=%llu\n",
            (unsigned long long)submitted, (unsigned long long)audible);
    running = false;
    port = -1;
    outputDevice = nullptr;
}
}
