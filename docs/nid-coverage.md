# NID coverage workflow

Automatic check that every import a game needs is either implemented or
tracked, replacing the previous by-hand comparison.

## Loop

1. Relink with diagnostics (from the repo root, after building `relinker`):
   `relinker --windows --windows-diagnostics <input.elf> <output.elf>`
   - `--windows-diagnostics` requires `--windows`
     (`core/relinker/cli/src/CliArgs.cpp`); the Linux target rejects it.
   - It writes `windows-diagnostics-imports.txt` (one NID per line) into the
     current working directory
     (`core/relinker/elfpatcher/src/windows/WindowsPePatcher.cpp`).
2. Run the check (no C++ build needed, Python 3 + stdlib only):
   `python3 tools/check-nid-coverage.py <windows-diagnostics-imports.txt>`
   - Defaults: `--json docs/legends-unresolved-nids.json`,
     `--prx-root core/libs/prx`.
3. Implement stubs for the reported `NEW` NIDs (or add them to the JSON if
   they are out of scope), then repeat from step 1.

## What the script does

- Loads the known-missing list: `docs/legends-unresolved-nids.json`
  (316 unique NIDs grouped by importer module, UTF-8 with BOM).
- Scans `core/libs/prx/*/Export.cpp` for export names: `APS5_VABI <name>(`
  definitions (following `#include "prx/.../Export.cpp"` includes, plus the
  `APS5_DUMMY_FUN` export). Both raw NIDs and `*_nid_postfix` names are
  handled: the postfix is stripped for comparison, and the base name is
  hashed with the same algorithm `nid_patcher` uses at build time
  (`core/libs/nid/src/NidCompute.cpp`, `NidPatcherUtils.hpp`), so comparison
  is obfuscated-NID to obfuscated-NID.
- Prints per-module covered-vs-missing counts, the status of each diagnostic
  NID (`COVERED` with providing prx, `TODO` with JSON location, `NEW`), and
  `STALE` JSON entries that are already provided.
- Exit status: `0` = no NEW NIDs, `1` = NEW NIDs found (neither provided nor
  in the JSON), `2` = usage/IO error.

## Limitations

- Static approximation of `nid_patcher`: build-time `--preserve-exports`
  exclusions are not modelled; the built `.prx` files remain the source of
  truth.
- Only top-level `core/libs/prx/*/Export.cpp` files are scanned (e.g. not
  `core/libs/prx/libc/src/exception/Exports.cpp`).
