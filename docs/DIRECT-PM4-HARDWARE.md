# Direct GPU command memory: hardware finding and candidates

## Physical evidence (2026-09-28)

The user's physical PS4 passed **Vulkan Probe 0.5**: raw EOP, init+EOP,
separate init/EOP submissions, Vulkan empty submission, DMA fill, memory barrier,
and all 64 readback words. Probe 0.4 had failed the Vulkan empty submission even
with SFENCE and SubmitDone. The distinguishing successful probe change copied
PM4 command bytes from ordinary heap storage into directly allocated GPU memory.
This is strong evidence for command-buffer memory accessibility as a cause of
that startup failure. It is not proof of full application rendering or gameplay.

## General fix

All three local native ICD variants now record PM4 directly into WC Garlic direct
memory with CPU/GPU read-write permissions and 64 KiB alignment. Buffers retain
a stable address through recording, submission and command-pool reuse. This
preserves commands that refer to data inside their own recording buffer; the
probe's limited relocation shortcut is removed from the UT99 driver.

Each command buffer owns its direct-memory handle. Pool destruction, cache trim,
free-list overflow and error cleanup release it through the direct-memory API.
Partial allocation failures remove tracked command buffers before freeing them.
Host-only builds retain their ordinary allocator. SFENCE publication is retained;
SubmitDone is retained/added at the existing completion boundaries. The projects'
other renderer features and queue behavior are not made byte-identical.

`scripts/test-pm4-lifetime.sh` tests the actual PS4 allocation code with tracked
host stubs and ASan/UBSan: allocation, stable reuse, destruction of active buffers,
trimming cached buffers, and rollback after a partial allocation failure. This
checks CPU ownership and lifetime, not real GPU execution.

## Candidates

| Application | Visible version | USB filename |
|---|---|---|
| ScummVM | ScummVM Vulkan Direct PM4 1.20 | ScummVM-PS4-Vulkan-Experimental.pkg |
| UT99 | UT99 Orbis Direct PM4 0.9 | ut99-orbis-hardware.pkg |
| Doom 3 | Doom 3 PS4 - Direct PM4 0.02 | Doom3-PS4-Menu-Test-0.02.pkg |
| Basic probe using general allocator | PS4 Vulkan Probe 0.6 | ut99-vulkan-probe.pkg |
| Log viewer | PS4 Vulkan Diagnose 0.11 | ut99-orbis-diagnostic.pkg |

Start the probe, then ScummVM's launcher, UT99 and Doom 3. On a crash open the
log viewer again to reload its snapshots; photograph the relevant engine/start
and GPU pages. The viewer now includes `/data/doom3-client/dudelog.txt` and
`/data/client-vulkan.log` (pages 7/8 and 8/8). Log files may be from older runs.
Do not relaunch a failing app before photographing its logs, as it may overwrite them.

Only Probe 0.5 has been physically verified at this checkpoint. The general
allocator and the three new application packages still need physical testing.
Probe 0.6 has no draw/shader tests. Doom 3 remains a finite 60-frame menu test
with a five-second capture hold, not a completed gameplay port.

## Rebuild Doom 3 and audit the actual package

```sh
scripts/package-ps4-client.py --game-data media/game
scripts/verify-ps4-package.py
scripts/test-pm4-lifetime.sh
CLIENT_EBOOT="$PWD/build/client-package-extracted/uroot/eboot.bin" \
 CLIENT_PROFILE="$PWD/build/package-direct-pm4-profile" \
 CLIENT_ARTIFACTS="$PWD/artifacts/package/direct-pm4-emulator" \
 CLIENT_USE_PACKAGED_DATA=1 \
 xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-ps4-client.py
```

The new 00.02 package has a distinct title/version and packages five retail
archives under `/app0/base`. Loader modules, icon, SFO and notices are included.
`build-info.json` records ELF, eboot, native ICD and patch hashes. Packaging
rejects an ELF without the new direct-PM4 marker. The extraction audit checks
all 17 inputs, including the retained SFO application fields. Emulator validation
must use packaged data with no external `doom3-game` link.

There is no established evidence that the earlier 0.01 PKG was structurally
corrupt. Its old documentation recorded a successful extraction and emulator
menu test. The new package is rebuilt and audited independently; the earlier
heap-backed GPU command-buffer defect also affected this renderer.

The new extracted package displayed the Doom 3 main menu, completed 60 engine
frames and exited 0 in shadPS4 using only packaged data. All 17 inputs passed
the extraction audit. Evidence: `artifacts/package/direct-pm4-emulator/` and
`artifacts/client/direct-pm4-package-audit.log`. This does not establish physical
rendering, input, audio or gameplay.

## Built package SHA-256

- `Doom3-PS4-Menu-Test-0.02.pkg`: `ecb98406ff85a0f87b47ff5286cc1bbc50134664067591d1546758b1cf1669c7`

## Clear External 0.03

Supersedes the bundled-data 0.02 candidate: see [package and test evidence](PS4_PACKAGE.md).
ScummVM 1.24 was reported working on physical PS4 with Thimbleweed Park.
UT99 0.11 and Doom 3 0.03 carry the same compiled color-clear approach, with
application-specific descriptor/push-state handling retained. Doom 3 passed
the extracted-package emulator menu test with external data; hardware pending.
