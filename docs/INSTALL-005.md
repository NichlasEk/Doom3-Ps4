# Installation candidate 0.05 — 2026-09-29

The user reports that 0.04 cannot be installed and confirms there is no older
Doom installation. Recopied USB package bytes matched the local source both
before and after copying, followed by sync and safe unmount. Copy corruption
is therefore not supported by the available evidence. No numerical console
installation error is available yet.

0.04 used TITLE_ID `DM3P00001`, with a digit in its four-character prefix.
0.05 changes it to `DOOM00001` and CONTENT_ID to
`IV0000-DOOM00001_00-DOOM3PS4MENUTEST`. The package follows the conventional
four-letter/five-digit title identity used by our working ScummVM/UT99 packages.
This is an installation hypothesis, not a confirmed diagnosis of the console
error. PkgTool had validated the previous package without rejecting its ID.
OpenOrbis packaging reference:
https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/docs/MD/Building%20Homebrew.md

The packaging script now rejects identities outside this format before building;
the extracted-package verifier checks the title/content ID pair, version and
manifest consistency. Version is `00.05`; all 13 payload/metadata checks pass.
The executable is unchanged from 0.04. No new runtime/gameplay claim is made.
Retail archives remain external at `DOOM3/base/pak000.pk4` through `pak004.pk4`.
Settings/saves still use `/data/doom3-client`. Audio remains disabled.

Install `dist/Doom3-PS4-Playable-Alpha-0.05.pkg` and report the exact error code
if installation still fails. Do not delete saves or data for this test.
The previous package is retained locally for comparison.

SHA-256: `920af2f82ab16d2df9fec03167d425867cc1c716a544df884083e83e0ad67394`.
Evidence: `artifacts/package/repack-005.log`, `verify-005.log`, `manifest.json`
and `extraction-check.json`. Physical install and USB delivery are pending.
