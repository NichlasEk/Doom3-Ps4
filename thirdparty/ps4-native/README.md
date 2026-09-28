# Native PS4 graphics stack

The build recipe, code generation helper, dependency pins, push-constant lowering,
and initial patches were copied from the local `ut99-orbis` project on 2026-09-28
(HEAD `3d5099ee1b799b2ab73e9343c5391f1df144a6cc`). The checkout had local
changes; the files stored here are the authoritative snapshot, rather than a
claim that everything comes from that commit. The initially untracked
`include/vk_ps4_pm4_chunks.h` is included in our Vulkan patch so a fresh build
has every required header.

Doom3-Ps4 modifications:

- Own build paths and `doom3_psbc_*` private thread symbols.
- Aggregate Vulkan 1.2/1.3 feature queries and aggregate device-feature validation.
- `VK_KHR_map_memory2`, including core and KHR entry points.

Port helper code retains the notice in [LICENSE.ut99-orbis](LICENSE.ut99-orbis).
Downloaded upstream sources retain their own licenses (including Mesa components
inside PSBC); the patches do not replace those licenses.

Run `scripts/build-native-vulkan.sh` from the repository root. This clones pinned
upstream sources, applies the checked-in patches, generates compiler sources,
and builds OpenGNM, PSBC and the Vulkan driver under ignored `build/native/`.
Logs and archive hashes are under ignored `artifacts/native/`. No binaries or
commercial Doom data are committed. The graphics stack is not yet linked to
the Doom engine diagnostic.

The shader milestone extends PSBC and the driver to two descriptor sets and
fixes swapchain renderpass clears. `tests/ps4_shader_probe.c` is derived from the
pinned Vulkan driver's triangle example; its MIT notice is retained in
[LICENSE.vulkan-ps4](LICENSE.vulkan-ps4). The modified example uses generated
SPIR-V and adds explicit Vulkan error checks, dynamic state, pixel-test
patterns and descriptor resources. DUDE shader source stays in the ignored
reference clone under its upstream license; generated binaries stay ignored.
