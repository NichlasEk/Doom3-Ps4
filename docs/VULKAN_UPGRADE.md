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

## Shader and GPU milestone — 2026-09-28

The next implementation step is complete:

- The bounded descriptor ABI now accepts **sets 0 and 1**, each with bindings
  0–15 and the previously supported UBO / fragment sampler2D resource types.
  PSBC emits one GNM input-usage slot per used descriptor set, with its set index
  in `apislot`. The driver binds the corresponding resource-table pointer for
  each shader stage. Layout validation uses separate masks for each set.
- Pipeline layouts accept two sets; separate `firstSet=0` and `firstSet=1` binds
  preserve each other's state. Command-buffer reset clears both bindings.
- Push-constant snapshots remain attached to set 0. This is a bounded graphics
  implementation, not support for arbitrary sets, descriptor arrays, dynamic
  descriptor offsets, or full Vulkan pipeline-layout compatibility validation.
- Renderpass color clears now handle VideoOut-backed swapchain images, which
  have no `VkDeviceMemory` wrapper. Previously the clear silently returned.

### Shader audit

`python3 scripts/audit-dude-shaders.py` builds the host PSBC compiler and audits
all graphics stages from the pinned DUDE reference. **172/172** compile and
validate as Vulkan 1.4 SPIR-V; **127/172** compile to Liverpool GCN binaries.
All six required `generic`, `zfill`, and `shadow` vertex/fragment stages pass.
The script's success means those six pass, not that all 172 are supported.

Detailed per-shader results, hashes and compiler diagnostics are in
`artifacts/native/dude-shader-audit.json`. Native binaries and individual logs
are in `build/dude-shaders/gcn`. PSBC's existing SPIR-V capability warnings are
retained in the logs; a compiler return code is not a general support claim.

### Actual emulator rendering

```sh
./scripts/build-shader-probe.sh demote
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py demote
./scripts/build-shader-probe.sh generic
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py generic
./scripts/test-vulkan-features.sh
```

Both probes cross-compile and run as native PS4 executables in shadPS4 v0.18.0,
revision `e3ce810f3a653f43ac64ebab63023de281a4103a`. The runner compares **every
pixel of the 1280×720 capture** with the expected image and requires guest exit 0.
Both passed with **921600 checked pixels, zero mismatches**:

- `demote`: Vulkan 1.4 SPIR-V, verified `OpDemoteToHelperInvocation`; odd pixel
  columns preserve the clear color and even columns are green only if `dFdx`
  still returns the expected derivative. Both descriptor sets are read in the
  fragment shader; set 0 is also read in the vertex shader.
- `generic`: **unmodified DUDE `generic.vert` and `generic.frag`**, using the
  upstream prelude/includes. A generated RenderParams block goes in set 0;
  a two-texel RGBA texture and nearest sampler go in set 1. Opaque green and
  transparent red texels produce alternating 16-pixel green/background stripes.
  This verifies real DUDE shader execution, texture sampling and alpha discard.

Captures and run results are under `artifacts/shader-probe-{demote,generic}/`.
The shader probe uses dynamic viewport/scissor state: the upstream sample's
static state was not emitted by the driver and initially produced a black
screen. The probe now declares and sets the dynamic states explicitly.

The ASan/UBSan host test also passed independent descriptor binding and rejected
out-of-range set updates without altering existing bindings.

### What this does not yet establish

The probe is a standalone graphics executable. The Doom engine still has its
dedicated diagnostic; no game menu, level or lighting is connected to this
renderer yet. Physical PS4 behavior is untested. The driver continues to report
API 1.1 and does not advertise demote as generally supported: the tested shader
paths do not cover all helper-invocation semantics and storage side effects.
See the [Khronos demote specification](https://github.com/KhronosGroup/SPIRV-Registry/blob/main/extensions/EXT/SPV_EXT_demote_to_helper_invocation.asciidoc)
for the full requirements.

The next blockers include `interaction.frag` and `ambientlight.frag`: the
current compiler path rejects cube/shadow sampling and texture operations beyond
its supported subset. The 45 rejected stages also include tessellation, BDA,
ray-tracing and other advanced shaders. Those failures remain visible in the
audit report. Completing the base lighting path and connecting the graphical
engine client are the next substantial steps.
