/* zlib licensed port code. Lower one push block to private set-0 UBO 15.
 * Explicit SPIR-V member offsets/strides stay intact. Native compiler accepts
 * only the bounded VS/PS UBO ABI; unsupported layouts must remain errors. */
#ifndef UT99_PS4_SPIRV_PUSH_CONSTANTS_H
#define UT99_PS4_SPIRV_PUSH_CONSTANTS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int ps4_lower_push_constants(const uint32_t *src, size_t bytes, uint32_t **out, size_t *out_bytes) {
    *out = NULL; *out_bytes = bytes;
    if (bytes < 20 || bytes % 4 || src[0] != 0x07230203) return -1;
    size_t n = bytes / 4, insert = 0;
    uint32_t variable = 0;
    for (size_t p = 5; p < n;) {
        uint32_t wc = src[p] >> 16, op = src[p] & 65535;
        if (!wc || wc > n - p) return -1;
        if (!insert && op >= 19 && op <= 39) insert = p;
        if (op == 59 && wc >= 4 && src[p+3] == 9) { /* OpVariable PushConstant */
            if (variable) return -1;
            variable = src[p+2];
        }
        p += wc;
    }
    if (!variable) return 0;
    if (!insert || bytes > SIZE_MAX - 32) return -1;
    uint32_t *dst = (uint32_t *)malloc(bytes + 32);
    if (!dst) return -1;
    memcpy(dst, src, insert*4);
    uint32_t decorations[8] = {(4u<<16)|71, variable, 34, 0, (4u<<16)|71, variable, 33, 15};
    memcpy(dst+insert, decorations, sizeof decorations);
    memcpy(dst+insert+8, src+insert, (n-insert)*4);
    for (size_t p = 5; p < n+8;) {
        uint32_t wc = dst[p] >> 16, op = dst[p] & 65535;
        if (op == 32 && wc == 4 && dst[p+2] == 9) dst[p+2] = 2; /* OpTypePointer Uniform */
        if (op == 59 && wc >= 4 && dst[p+3] == 9) dst[p+3] = 2;
        p += wc;
    }
    *out = dst; *out_bytes = bytes + 32;
    return 1;
}
#endif
