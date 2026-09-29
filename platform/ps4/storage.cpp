#include "storage.h"
#include <orbis/libkernel.h>
#include "libjbc.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>

namespace ps4 {
namespace {
unsigned ownedMounts;

bool gameReadable(const char* folder) {
    char file[600];
    const int length = std::snprintf(file, sizeof(file), "%s/base/pak000.pk4", folder);
    if (length < 0 || size_t(length) >= sizeof(file)) return false;
    int fd = sceKernelOpen(file, O_RDONLY, 0);
    if (fd < 0) return false;
    char header[2]{};
    auto bytes = sceKernelRead(fd, header, sizeof(header));
    sceKernelClose(fd);
    return bytes == 2 && header[0] == 'P' && header[1] == 'K';
}

bool restore(const jbc_cred& original) {
    // Do not continue to game startup when restoration fails.
    for (int attempt = 0; attempt < 3; ++attempt)
        if (!jbc_set_cred(&original)) return true;
    sceKernelDebugOutText(0, "Doom3 USB: credential restoration failed\n");
    return false;
}

StorageResult result(StorageStatus status, const char* path = "") {
    StorageResult out;
    out.status = status;
    std::snprintf(out.path, sizeof(out.path), "%s", path);
    char message[600];
    std::snprintf(message, sizeof(message), "Doom3 USB: status=%d path=%s\n", int(status), out.path);
    sceKernelDebugOutText(0, message);
    return out;
}
}

StorageResult resolveGameData(const char* preferred, bool allowSandboxMapping) {
    if (!preferred || preferred[0] != '/' || std::strlen(preferred) >= 480)
        return result(StorageStatus::InvalidPath);
    if (gameReadable(preferred)) return result(StorageStatus::Ready, preferred);
    // An explicit internal/custom path must not silently select unrelated USB data.
    if (std::strncmp(preferred, "/mnt/usb", 8) || preferred[8] < '0' || preferred[8] > '7' || preferred[9] != '/')
        return result(StorageStatus::NotFound);
    const char* suffix = preferred + 10;
    char candidates[8][512];
    char aliases[8][512];
    for (int slot = 0; slot < 8; ++slot) {
        std::snprintf(candidates[slot], sizeof(candidates[slot]), "/mnt/usb%d/%s", slot, suffix);
        std::snprintf(aliases[slot], sizeof(aliases[slot]), "/doom3_usb%d/%s", slot, suffix);
        if (gameReadable(candidates[slot])) return result(StorageStatus::Ready, candidates[slot]);
        if (gameReadable(aliases[slot])) return result(StorageStatus::Ready, aliases[slot]);
    }
    if (!allowSandboxMapping) return result(StorageStatus::NotFound);

    jbc_cred original{};
    if (jbc_get_cred(&original)) return result(StorageStatus::ServiceUnavailable);
    jbc_cred external = original;
    if (jbc_jailbreak_cred(&external) || jbc_set_cred(&external)) {
        return result(restore(original) ? StorageStatus::AccessFailed : StorageStatus::RestoreFailed);
    }
    unsigned present = 0;
    for (int slot = 0; slot < 8; ++slot)
        if (gameReadable(candidates[slot])) present |= 1u << slot;
    if (!restore(original)) return result(StorageStatus::RestoreFailed);
    if (!present) return result(StorageStatus::NotFound);

    for (int slot = 0; slot < 8; ++slot) {
        if (!(present & (1u << slot))) continue;
        char source[32], alias[32];
        std::snprintf(source, sizeof(source), "/mnt/usb%d", slot);
        std::snprintf(alias, sizeof(alias), "doom3_usb%d", slot);
        const int error = jbc_mount_in_sandbox(source, alias);
        if (!error) ownedMounts |= 1u << slot;
        // The helper temporarily changes credentials too, including on errors.
        if (!restore(original)) return result(StorageStatus::RestoreFailed);
        if (gameReadable(aliases[slot])) return result(StorageStatus::Ready, aliases[slot]);
    }
    return result(StorageStatus::MountFailed);
}

void releaseGameDataMounts() {
    if (!ownedMounts) return;
    jbc_cred original{};
    if (jbc_get_cred(&original)) return;
    for (int slot = 0; slot < 8; ++slot) {
        if (!(ownedMounts & (1u << slot))) continue;
        char name[32];
        std::snprintf(name, sizeof(name), "doom3_usb%d", slot);
        const int error = jbc_unmount_in_sandbox(name);
        if (!restore(original)) return;
        if (!error) ownedMounts &= ~(1u << slot);
    }
}
}
