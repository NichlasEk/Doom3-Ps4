# Vulkan upgrade toward the DUDE renderer

## Completed first step — 2026-09-28

A separate, source-built native graphics stack now lives in this project. Its
pinned dependencies and inherited UT99 patches are documented in
[thirdparty/ps4-native](../thirdparty/ps4-native/README.md).

New driver behavior:

- `VkPhysicalDeviceVulkan12Features` reports the same values as the existing
  extension feature queries. All other members are explicitly false.
- `VkPhysicalDeviceVulkan13Features` is initialized to false. Both queries
  preserve the `pNext` chain, including unknown structures.
- Device creation rejects unsupported requested base/1.1/1.2/1.3 features with
  `VK_ERROR_FEATURE_NOT_PRESENT` before allocating GPU resources. This is not
  comprehensive validation of every extension-specific feature structure.
- `VK_KHR_map_memory2` is advertised. The core and KHR map/unmap entry points
  use the existing PS4 direct-memory backend, including bounds checks and
  `VK_WHOLE_SIZE`. Placed mapping / address reservation are not advertised and
  their flags are rejected.

The driver still reports Vulkan **1.1**. This change does not establish Vulkan
1.4 conformance or make unmodified DUDE start. Existing extension feature claims
are inherited from the UT99 driver and require separate GPU validation.

## Evidence

```sh
./scripts/build-native-vulkan.sh
./scripts/test-vulkan-features.sh
```

The entire stack cross-compiled with OpenOrbis. The resulting renamed ICD archive
exports both core and KHR map-memory2 entry points. Archive hashes are recorded
in `artifacts/native/archives.sha256`.

The host regression executable links actual driver code, uses synthetic physical
and memory objects, and runs with AddressSanitizer and UndefinedBehaviorSanitizer.
It checks poisoned query structures, DUDE's 1.3 → 1.2 → 1.1 chain, unknown-node
preservation, extension parity, feature rejection, mapping offsets and bounds,
unmapping, and unsupported mapping flags. It passed. This is CPU-side driver
validation, not a GPU rendering or physical PS4 test. The earlier dedicated
Doom engine emulator result is independent of this graphics work.

## Renderer requirements and next gates

Reference: [DUDE](https://github.com/Inkub0/dude), inspected commit
`3b6872fe278802496cc7a8f8209b847c68efed2a`,
`neo/renderer/rhi/vk/VulkanBackend.cpp`. The local reference clone is ignored at
`.tools/dude-reference`.

| Area | Observed requirement | Remaining work |
| --- | --- | --- |
| Startup | Rejects API below 1.4; VMA configured for 1.4 | Implement prerequisites and validate before changing the reported version |
| Shaders | Vulkan 1.4 compilation target; discard uses demote-to-helper | Compile representative DUDE shaders to Liverpool GCN; verify derivatives, helper lanes, discarded writes on GPU |
| Main graphics | Traditional renderpasses, barriers, draws, copies, blits | Exercise each path with Doom materials and image/depth formats |
| Optional features | Descriptor indexing, indirect count, BDA, float16, separate depth/stencil layouts | Test feature-gated fallback paths and each enabled capability |
| Optional ray tracing | Acceleration structures, ray queries, deferred host operations | Keep disabled while unsupported |
| Platform | SDL Vulkan surface, swapchain, input and audio | Connect to PS4 VideoOut and engine client loop |

Full Vulkan 1.4 has additional mandatory capabilities beyond this renderer's
observed call sites. The [Khronos 1.4 description](https://docs.vulkan.org/features/latest/features/proposals/VK_VERSION_1_4.html)
is the checklist source for the broader upgrade. Dynamic rendering, synchronization2,
maintenance functionality, shader/storage features, feature/property reporting,
and required limits still need implementation review and tests. Available
function names alone are not evidence of correct GPU behavior.
