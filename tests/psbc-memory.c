// Repeated compilation must release temporary NIR and preserve output bytes.
#include "psbc_compile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static long rss(void) {
    long total, resident;
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f || fscanf(f, "%ld %ld", &total, &resident) != 2) exit(2);
    fclose(f);
    return resident * sysconf(_SC_PAGESIZE);
}
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 2;
    fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f);
    void *input = malloc(size);
    if (!input || fread(input, 1, size, f) != (size_t)size) return 2;
    fclose(f);
    PsbcCompileOptions opts = {0};
    opts.target = PSBC_TARGET_PS4_BASE;
    opts.stage = PSBC_STAGE_VERTEX; opts.entrypoint = "main"; opts.optimise = true;
    opts.uniform_binding_mask = 0xffff;
    void *reference = NULL; size_t reference_size = 0;
    long warm = 0;
    psbc_init();
    for (int i = 0; i < 450; ++i) {
        PsbcShaderOutput out = {0};
        if (psbc_compile_shader(input, size, &opts, &out) != PSBC_RESULT_OK) return 3;
        if (!i) {
            reference_size = out.size; reference = malloc(out.size);
            memcpy(reference, out.data, out.size);
        } else if (out.size != reference_size || memcmp(reference, out.data, out.size)) return 4;
        psbc_free_output(&out);
        if (i == 49) warm = rss();
    }
    long growth = rss() - warm;
    printf("compiles=450 identical_outputs=1 rss_growth_after_warmup=%ld bytes\n", growth);
    psbc_shutdown(); free(reference); free(input);
    return growth > 16 * 1024 * 1024 ? 1 : 0;
}
