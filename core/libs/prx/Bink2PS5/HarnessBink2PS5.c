// Tiny reusable PRX stub harness (copy pattern for other prx branches).
//
// What it does:
//   1. LoadLibraryExA the PATCHED prx (LOAD_WITH_ALTERED_SEARCH_PATH so sibling
//      prx deps such as libc.prx/libkernel.prx resolve from the same folder).
//   2. GetProcAddress every expected NID export, fail if any is missing.
//   3. Call 2 stubs (zero-arg, SysV ABI like the prx exports) and check they
//      return 0.
//
// This file is NOT part of the prx build: core/libs/CMakeLists.txt only
// compiles ${LIB_DIR}/Export.cpp for prx dirs without their own
// CMakeLists.txt, so this file is ignored by the build.
//
// Build (PowerShell, GCC first on PATH, no '&' char):
//   $env:PATH = "C:\codes\Anyps5\winlibs-gcc15\mingw64\bin;" + $env:PATH
//   gcc -O2 -o build\Bink2PS5_harness.exe core\libs\prx\Bink2PS5\HarnessBink2PS5.c
//
// Run (from the repo root, after `cmake --build build --target libs`):
//   .\build\Bink2PS5_harness.exe build\core\libs\libs\Bink2PS5.prx
// Expected: "18/18 NIDs resolved, 2/2 stub calls returned 0" and exit code 0.
// The stubs log "Bink2PS5 stub <NID> called" to stderr via APS5_LOG_ERR.
//
// HOW TO COPY FOR ANOTHER PRX (e.g. libcohtml, libfmod):
//   1. Copy this file next to the new prx's Export.cpp, rename it.
//   2. Replace PRX_LABEL, the NIDS[] list and EXPECTED_NIDS with the new lib's
//      NIDs (see docs/legends-unresolved-nids.json).
//   3. Pick any 2 NIDs for CALL_NID_0/CALL_NID_1 (prefer one containing '+' or
//      '-' to prove special-char exports resolve).
//   4. Adjust the gcc -o / exe / prx path lines above.

#include <stdio.h>
#include <windows.h>

// Prx stubs use SysV ABI on Windows (APS5_VABI, see
// prx/libc/include/general/VabiMacros.hpp), so the caller pointer must too.
// Zero-arg uint64-return calls happen to work without it, but declare it to
// stay correct if stubs ever take arguments.
typedef unsigned long long (__attribute__((sysv_abi)) *stub_fn_t)(void);

#define PRX_LABEL "Bink2PS5"
#define EXPECTED_NIDS 18

static const char *NIDS[EXPECTED_NIDS] = {
    "AOzr4mF9mLY",
    "B5o-N7kNTxs",
    "bLejwFWcUqs",
    "BmbetWVT5mw",
    "DnuAcNI4w8A",
    "E5jwXp52zHE",
    "ivBHf-nRkgY",
    "IXgFcanS2v4",
    "jRGnyN0asN0",
    "kGq53EISh-0",
    "m03EhZlYZew",
    "MSSY9bBRxHY",
    "NYHnUggK-hA",
    "pO5SeoJbRt0",
    "TJyfOXNz7a4",
    "UOUpNbLHx2g",
    "wse4gunZeJU",
    "yfz-zjkJDkE",
};

// Two stubs exercised for return value (first + one with '-' in its NID).
#define CALL_NID_0 "AOzr4mF9mLY"
#define CALL_NID_1 "B5o-N7kNTxs"

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : ("build\\core\\libs\\libs\\" PRX_LABEL ".prx");
    HMODULE mod = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!mod) {
        printf("FAIL: LoadLibraryExA(%s) error %lu\n", path, GetLastError());
        return 1;
    }
    printf("loaded %s\n", path);

    int resolved = 0;
    for (int i = 0; i < EXPECTED_NIDS; i++) {
        FARPROC p = GetProcAddress(mod, NIDS[i]);
        if (!p) {
            printf("FAIL: GetProcAddress(%s) error %lu\n", NIDS[i], GetLastError());
            FreeLibrary(mod);
            return 1;
        }
        resolved++;
    }
    printf("%d/%d NIDs resolved\n", resolved, EXPECTED_NIDS);

    const char *calls[2] = { CALL_NID_0, CALL_NID_1 };
    for (int i = 0; i < 2; i++) {
        stub_fn_t fn = (stub_fn_t)(void *)GetProcAddress(mod, calls[i]);
        if (!fn) {
            printf("FAIL: GetProcAddress(%s) error %lu\n", calls[i], GetLastError());
            FreeLibrary(mod);
            return 1;
        }
        unsigned long long rc = fn();
        printf("call %s -> %llu\n", calls[i], rc);
        if (rc != 0) {
            printf("FAIL: stub %s returned %llu, expected 0\n", calls[i], rc);
            FreeLibrary(mod);
            return 1;
        }
    }

    printf("OK: %d/%d NIDs resolved, 2/2 stub calls returned 0\n", resolved, EXPECTED_NIDS);
    FreeLibrary(mod);
    return 0;
}
