#!/usr/bin/env python3
"""Check NID coverage of a relinked game against the known-missing list.

Workflow (see docs/nid-coverage.md):
  1. relinker --windows --windows-diagnostics <input.elf> <output.elf>
     writes ``windows-diagnostics-imports.txt`` (one NID per line) into the
     current working directory
     (core/relinker/elfpatcher/src/windows/WindowsPePatcher.cpp).
  2. python3 tools/check-nid-coverage.py <windows-diagnostics-imports.txt>
  3. implement stubs for the reported NEW NIDs, then repeat from step 1.

Exit status:
  0 - every diagnostic NID is either provided by an existing prx export or
      listed in the known-missing JSON (known-missing, to be implemented).
  1 - at least one diagnostic NID is NEW: neither provided nor known-missing.
  2 - usage or IO error (missing file, bad JSON, ...).
"""

import argparse
import base64
import hashlib
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_JSON = REPO_ROOT / "docs" / "legends-unresolved-nids.json"
DEFAULT_PRX_ROOT = REPO_ROOT / "core" / "libs" / "prx"

NID_SUFFIX = bytes.fromhex("518d64a635ded8c1e6b039b1c3e55230")

NID_POSTFIX = "_nid_postfix"
NID_NO_PATCH = "_nid_no_patch"
NID_NO_PATCH_CUT = "_nid_no_patch_cut"
DISAMBIG_MARKER = "_nid_disambig"
SDL_PREFIX = "SDL_"
DUMMY_MACRO = "APS5_DUMMY_FUN"
DUMMY_EXPORT = "DummyFunction_nid_no_patch"

EXPORT_RE = re.compile(r"APS5_VABI\s+(\w+)\s*\(")
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
NID_LIKE_RE = re.compile(r"^[A-Za-z0-9+\-]{11}$")
DISAMBIG_RE = re.compile(r"_nid_disambig\d+$")


def compute_nid(symbol):
    """Obfuscated NID for a base symbol name.

    Mirrors core/libs/nid/src/NidCompute.cpp (same salt, same sha1 +
    base64-with-``+-`` alphabet encoding); kept in sync with the ``nid()``
    helper in core/libs/tests/run_exception_tests.py.
    """
    digest = hashlib.sha1(symbol.encode() + NID_SUFFIX).digest()
    return base64.b64encode(digest[:8][::-1]).decode().rstrip("=").replace("/", "-")


def is_no_patch_cut(name):
    return name.endswith(NID_NO_PATCH_CUT)


def is_no_patch(name):
    return name.endswith(NID_NO_PATCH) or name.startswith(SDL_PREFIX)


def strip_postfix(name):
    """Mirror of Nid::Internal::StripNidPostfix (NidPatcherUtils.hpp)."""
    result = name
    if result.endswith(NID_POSTFIX):
        result = result[: -len(NID_POSTFIX)]
    result = DISAMBIG_RE.sub("", result)
    return result


def read_export_file(path, seen):
    """Read an Export.cpp plus any ``#include "prx/.../Export.cpp"`` it pulls in."""
    path = path.resolve()
    if path in seen:
        return ""
    seen.add(path)
    try:
        text = path.read_text(encoding="utf-8-sig")
    except OSError as exc:
        print(f"warning: cannot read {path}: {exc}", file=sys.stderr)
        return ""
    chunks = [text]
    for include in INCLUDE_RE.findall(text):
        if include.endswith("Export.cpp"):
            candidate = (path.parent / include).resolve()
            if candidate.is_file():
                chunks.append(read_export_file(candidate, seen))
            else:
                repo_candidate = (REPO_ROOT / "core" / "libs" / include).resolve()
                if repo_candidate.is_file():
                    chunks.append(read_export_file(repo_candidate, seen))
    return "\n".join(chunks)


def scan_prx_exports(prx_root):
    """Scan core/libs/prx/*/Export.cpp for export names.

    Returns (provided, providers) where ``provided`` maps every NID-or-name
    the built .prx files provide to True, and ``providers`` maps it to the
    list of "prx_lib:export_symbol" strings. Both raw NIDs and
    ``*_nid_postfix`` names are handled: the postfix is stripped for
    comparison (and the base name is hashed with compute_nid, mirroring what
    nid_patcher does at build time).
    """
    provided = {}
    for export_cpp in sorted(prx_root.glob("*/Export.cpp")):
        lib = export_cpp.parent.name
        text = read_export_file(export_cpp, set())
        names = EXPORT_RE.findall(text)
        if DUMMY_MACRO in text:
            names.append(DUMMY_EXPORT)

        name_set = set(names)
        for name in names:
            if is_no_patch_cut(name):
                final = name[: -len(NID_NO_PATCH_CUT)]
                add_provided(provided, final, lib, name)
            elif is_no_patch(name):
                add_provided(provided, name, lib, name)
            elif NID_LIKE_RE.match(name):
                add_provided(provided, name, lib, name)
            else:
                base = strip_postfix(name)
                has_postfix = base != name
                if not has_postfix and (name + NID_POSTFIX) in name_set:
                    add_provided(provided, name, lib, name)
                else:
                    add_provided(provided, compute_nid(base), lib, name)
                    add_provided(provided, base, lib, name)
    return provided


def add_provided(provided, key, lib, symbol):
    provided.setdefault(key, []).append(f"{lib}:{symbol}")


def load_known_missing(json_path):
    doc = json.loads(json_path.read_text(encoding="utf-8-sig"))
    known = {}
    for module in doc.get("modules", []):
        importer = module.get("importer", "?")
        for lib in module.get("libs", []):
            for entry in lib.get("nids", []):
                nid = entry.get("nid", "")
                known.setdefault(nid, []).append(
                    (importer, lib.get("lib", "?"), lib.get("modver", "?"), entry.get("kind", "?"))
                )
    return doc, known


def load_diagnostics(path):
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    diags = []
    for line in lines:
        nid = line.strip()
        if nid:
            diags.append(nid)
    unique = list(dict.fromkeys(diags))
    return unique, len(diags) - len(unique)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Validate relinker --windows-diagnostics NIDs against "
        "the known-missing JSON and existing prx exports."
    )
    parser.add_argument("diagnostics", type=Path, help="path to windows-diagnostics-imports.txt")
    parser.add_argument("--json", type=Path, default=DEFAULT_JSON, help="known-missing NID list")
    parser.add_argument("--prx-root", type=Path, default=DEFAULT_PRX_ROOT, help="core/libs/prx directory")
    args = parser.parse_args(argv)

    if not args.diagnostics.is_file():
        print(f"error: diagnostics file not found: {args.diagnostics}", file=sys.stderr)
        return 2
    if not args.json.is_file():
        print(f"error: known-missing JSON not found: {args.json}", file=sys.stderr)
        return 2
    if not args.prx_root.is_dir():
        print(f"error: prx root not found: {args.prx_root}", file=sys.stderr)
        return 2
    try:
        doc, known = load_known_missing(args.json)
    except (OSError, ValueError) as exc:
        print(f"error: cannot parse {args.json}: {exc}", file=sys.stderr)
        return 2

    provided = scan_prx_exports(args.prx_root)
    diags, dupes = load_diagnostics(args.diagnostics)

    covered, known_missing, new = [], [], []
    for nid in diags:
        if nid in provided:
            covered.append(nid)
        elif nid in known:
            known_missing.append(nid)
        else:
            new.append(nid)

    print(f"diagnostics : {args.diagnostics} ({len(diags)} unique, {dupes} duplicates skipped)")
    print(f"known-missing: {args.json} ({len(known)} unique NIDs)")
    print(f"prx exports : {args.prx_root} ({len(provided)} provided names/NIDs)")
    print(f"covered: {len(covered)}, known-missing: {len(known_missing)}, NEW: {len(new)}")
    print()

    print("== per-module coverage (known-missing JSON vs prx exports) ==")
    for module in doc.get("modules", []):
        importer = module.get("importer", "?")
        mod_total = mod_covered = 0
        mod_missing = []
        for lib in module.get("libs", []):
            for entry in lib.get("nids", []):
                nid = entry.get("nid", "")
                mod_total += 1
                if nid in provided:
                    mod_covered += 1
                else:
                    mod_missing.append((nid, lib.get("lib", "?"), entry.get("kind", "?")))
        print(f"[{importer}] covered {mod_covered}/{mod_total}, still missing {len(mod_missing)}")
        for nid, lib, kind in mod_missing:
            print(f"  MISSING {nid}  ({lib}, {kind})")
    print()

    if covered:
        print("== diagnostics: covered by existing prx ==")
        for nid in covered:
            print(f"  COVERED {nid}  <- {', '.join(provided[nid])}")
        print()
    if known_missing:
        print("== diagnostics: known-missing (in JSON, stub to implement) ==")
        for nid in known_missing:
            locs = "; ".join(f"{imp}/{lib} {modver} [{kind}]" for imp, lib, modver, kind in known[nid])
            print(f"  TODO {nid}  ({locs})")
        print()

    stale = sorted(nid for nid in known if nid in provided)
    if stale:
        print("== JSON entries already provided (consider removing from JSON) ==")
        for nid in stale:
            print(f"  STALE {nid}  <- {', '.join(provided[nid])}")
        print()

    if new:
        print("== diagnostics: NEW NIDs (neither provided nor known-missing) ==")
        for nid in new:
            print(f"  NEW {nid}")
        print()
        print(f"FAIL: {len(new)} NEW NID(s). Add them to {args.json} or implement the exports, then repeat.")
        return 1

    print("OK: no NEW NIDs. Every diagnostic import is provided or tracked as known-missing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
