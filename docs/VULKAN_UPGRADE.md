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

## Cubemap and ambient-light milestone — 2026-09-28

The next native path now executes **unmodified DUDE `ambientlight.vert` and
`ambientlight.frag`** in shadPS4. The fixture supplies its RenderParams, tangent
basis, normal map, diffuse texture, light falloff/projection textures and an
ambient cubemap. With the tested parameters the expected output is `(0,128,128)`.
Every pixel in the 1280×720 capture matches that result; guest exit is zero.

A separate `cube` probe uploads six differently colored faces and samples the
six principal directions in six screen bands. All **921600 pixels** match the
expected face order. The order follows the
[Khronos resource specification](https://docs.vulkan.org/spec/latest/chapters/resources.html):
+X, -X, +Y, -Y, +Z, -Z.

```sh
./scripts/build-shader-probe.sh cube
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py cube
./scripts/build-shader-probe.sh ambient
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py ambient
```

The implementation includes:

- PSBC support for non-array, non-shadow fragment cube samplers through the
  existing per-set texture/sampler descriptor tables.
- A bounded Vulkan cube image/view path: one square RGBA8 UNORM cube, six faces,
  one mip, optimal tiling, sampled + transfer-destination usage, full cube view
  with identity swizzle. Other cube shapes/usages/views are rejected.
- Buffer-to-image upload using one face per copy region. The face index now
  participates in tiled address calculation, including the bank rotation.
- An OpenGNM fix: micro/macro-tiled address routines now receive the sample count
  in their `numSamples` argument. Previously they received padded image depth.
  This broke addressing as soon as a multi-layer context was used.

The first cube test exposed both upload problems: only face zero was visible;
linear face offsets missed the bank rotation, and the subsequent layer-aware
address calculation exposed the wrong sample-count argument. The final test
covers all six faces. The current fixture uses one texel per face; larger faces,
face-edge filtering, mip chains, compressed cubes and physical PS4 rendering
remain unverified. Cube arrays, per-face views, shadow comparisons and cube
render targets are outside this initial implementation. The GPU fixture also
checks explicit rejection of cube arrays and per-face 2D views.

The shader audit now compiles **132/172** graphics stages to Liverpool GCN
(172/172 valid Vulkan 1.4 SPIR-V). The five newly compiling stages are
`ambientlight.frag`, `bumpyenvironment.frag`, `diffusecube.frag`,
`environment.frag`, and `skybox.frag`. Only the ambient shader pair and the
separate cube fixture were exercised on the emulator in this step.

The generic GUI/alpha-test pixel probe and the ASan/UBSan driver tests are
regression gates for this change. Results are retained under
`artifacts/shader-probe-{cube,ambient,generic}/` and `artifacts/native/`.

This establishes a standalone ambient-light pass, not engine integration or a
lit Doom level. Direct-light `interaction.frag` still requires shadow sampling
and additional texture operations. Full Vulkan 1.4 and the graphical engine
client remain unfinished.

## Direct-light and sampled-shadow milestone — 2026-09-28

DUDE's unmodified `interaction.vert`/`interaction.frag` now execute in shadPS4.
The standalone fixture binds all fifteen set-1 texture slots, uses a constant
normalization cube, RXGB normal map, gray diffuse map and white light projection
and falloff. Specular contribution and advanced material effects are disabled.
Two fixtures exercise the same original shader modules:

- `interaction`: unshadowed diffuse output `(128,128,128)` everywhere.
- `interactionshadow`: projected-shadow mode 1, a D32 map containing 0.5,
  and a reference depth that increases horizontally. The left half is gray;
  the right half is black. The shader executes its four comparison taps with
  zero spread; this does not validate bilinear or wide-kernel PCF.

Both check every one of 921600 pixels and exit with guest status zero.
Separate passing pixel fixtures are:

- `shadow`: two D32 texels (0.25/0.75), 16-pixel stripes, and three reference
  values (0.125/0.5/0.875) in horizontal bands. Nearest `LESS` comparisons
  produce green for lit pixels and red for shadowed pixels.
- `cubeshadow`: six uploaded D32 faces alternating 0.25/0.75, six principal
  directions and the same three reference values. All six faces are checked.
- `texops`: integer `texelFetch`, `textureSize` and explicit `textureGrad`
  agree on a two-texel RGBA8 image. The fixture has one mip; gradient-driven
  mip selection and minification remain unverified.

```sh
./scripts/build-shader-probe.sh interactionshadow
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py interactionshadow
```

The compiler's resource gate now accepts float fragment shadow comparisons,
explicit gradients and texel fetches through the existing Mesa NIR/ACO path.
Fetches and size queries use one texture descriptor; sampled operations also
require a sampler descriptor. Set/binding bounds and descriptor-array rejection
remain. Vulkan image creation and upload now support sampled D32 single-mip
cubes and host-linear sampled D32 2D images, with the depth aspect selected.
Cube attachment usage and face views remain rejected. Comparison uses the
existing GNM sampler mapping, following the
[Khronos comparison rules](https://docs.vulkan.org/spec/latest/chapters/textures.html).

The audit now reports **143/172** stages compiled to Liverpool GCN; all 172
still validate as Vulkan 1.4 SPIR-V. The required compilation gate is expanded
to ten stages: generic, zfill, shadow, ambientlight and interaction pairs.
Success also requires every required source to be present. Other stages that
compile are not thereby GPU-validated.

Depth data in these tests is supplied by the CPU or a staging upload. This
milestone validates the light receiver and sampling path; a shadow-caster
render pass, depth-target-to-sampled-image synchronization/layout, larger/mipped
shadow maps, PCF filtering and a real scene remain future work. No physical
PS4 or graphical Doom client was tested. The driver continues to report Vulkan
1.1; this is not complete Vulkan 1.4 support.

Reproduction artifacts are in `artifacts/shader-probe-{shadow,cubeshadow,texops,interaction,interactionshadow}/`.
The generic and ambient probes and ASan/UBSan driver checks are regression gates.

## Geometry-produced shadow map milestone — 2026-09-28

The `shadowcast` probe now renders a **128×64 D32 depth attachment from actual
triangle geometry**, then samples that same attachment in DUDE's unchanged
`interaction.vert`/`interaction.frag`. No CPU depth upload supplies this map.

The caster draws a near quad at depth 0.25 followed by an overlapping far quad
at depth 0.75 with depth test/write enabled and `LESS`. The light pass compares
against 0.5, so accepting the far quad incorrectly would erase the shadow.
The attachment clears to 1.0 before each of six frames. Alternating viewport
positions move the projected quad; the last capture also checks that the
previous frame's rectangle has been cleared. An unrelated 1×1 viewport is set
before beginning the depth pass, verifying that the load-op clear does not
inherit that dynamic drawing state.

All **921600 pixels** match the final expected image: a black rectangle at
`480 <= x < 1120`, `180 <= y < 540`, surrounded by `(128,128,128)` lighting.
Guest exit is zero. This is an emulator check of geometry, depth testing,
clearing, reuse, synchronization and sampling together.

Two defects in the inherited image path were fixed:

- A depth target's texture descriptor used a display/color tile mode instead
  of the actual depth target's tile mode and padded pitch.
- Binding depth memory set DB addresses but left the sampled texture's base
  address unset.

Depth creation now reports allocation failure if GNM target creation fails,
instead of silently replacing it with a texture. Combined sampled/depth
attachments are bounded to optimal-tiled, single-layer, single-mip D32 2D
images with full identity depth views. The fixture checks rejection of D16,
multiple layers/mips and a color-aspect view. HTILE compression is disabled.
The existing conservative command-buffer barrier flushes/waits for depth
writes before fragment texture reads; the fixture uses the early/late fragment
test stages, depth-write access and a transition to shader-read layout,
following the [Khronos synchronization example](https://docs.vulkan.org/guide/latest/synchronization_examples.html).

```sh
./scripts/build-shader-probe.sh shadowcast
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py shadowcast
```

Artifacts: `artifacts/shader-probe-shadowcast/{capture.png,result.json,emulator.log}`.
The sampled-depth `interactionshadow` and generic GUI probes, plus ASan/UBSan
CPU driver tests, are regression gates. The shader audit remains 143/172 with
all ten required stages passing.

This is a small synthetic geometric scene with custom caster shaders and the
original DUDE light receiver. It does not yet load a Doom level or integrate
the graphical client. Cube shadow attachments, compressed depth, larger scene
workloads, filtered PCF and physical PS4 rendering remain unverified or
unsupported. Vulkan 1.4 support remains incomplete.

## Graphical client integration

The private Vulkan 1.1 path is now connected to a separate DUDE PS4 client.
Dynamic UBO snapshots, bounded depth-plane copies and 1:1 VideoOut transfers
allow the real main menu to render in shadPS4. The menu diagnostic completes
60 engine frames. This is not Vulkan 1.4 support or gameplay validation.
See [client status](PS4_CLIENT.md), including the failing tiled-color-clear
probe and the slow DMA-based presentation path.
