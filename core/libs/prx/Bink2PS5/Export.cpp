// Bink2PS5 stub prx.
//
// Minecraft Legends' eboot.bin imports lib Bink2PS5_v1 (18 NIDs, see
// docs/legends-unresolved-nids.json). The real Bink video internals are
// unknown here, so every export is a stub in the spirit of KytyPS5's
// UnresolvedImportStub: log the call and return 0. This unblocks boot;
// video will be black/skipped (real Bink decoding is out of scope).
//
// Mechanism: the clear names behind these NIDs are unknown, so the raw
// obfuscated NID strings are exported via APS5_EXPORT (see
// prx/libc/include/general/ExportMacros.hpp). It emits an asm alias named
// "<NID>_nid_no_patch_cut"; nid_patcher (core/libs/nid, NidResolver.cpp /
// NidPatcherUtils.hpp IsNidNoPatchCut) strips that suffix and keeps the NID
// verbatim instead of hashing it. Quoted asm names also cover NIDs
// containing '+'/'-', which are not valid C++ identifiers.
//
// Note: UOUpNbLHx2g is imported without PLT, but its kind is still Func, so
// it is stubbed as a function like the others (a function address satisfies
// both call and address-taken imports).
#include <cstdint>
#include <cstdio>
#include "prx/libc/include/General.hpp"

extern "C" {

APS5_EXPORT("AOzr4mF9mLY", bink2ps5_stub_00);
uint64_t APS5_VABI bink2ps5_stub_00(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "AOzr4mF9mLY");
    return 0;
}

APS5_EXPORT("B5o-N7kNTxs", bink2ps5_stub_01);
uint64_t APS5_VABI bink2ps5_stub_01(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "B5o-N7kNTxs");
    return 0;
}

APS5_EXPORT("bLejwFWcUqs", bink2ps5_stub_02);
uint64_t APS5_VABI bink2ps5_stub_02(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "bLejwFWcUqs");
    return 0;
}

APS5_EXPORT("BmbetWVT5mw", bink2ps5_stub_03);
uint64_t APS5_VABI bink2ps5_stub_03(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "BmbetWVT5mw");
    return 0;
}

APS5_EXPORT("DnuAcNI4w8A", bink2ps5_stub_04);
uint64_t APS5_VABI bink2ps5_stub_04(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "DnuAcNI4w8A");
    return 0;
}

APS5_EXPORT("E5jwXp52zHE", bink2ps5_stub_05);
uint64_t APS5_VABI bink2ps5_stub_05(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "E5jwXp52zHE");
    return 0;
}

APS5_EXPORT("ivBHf-nRkgY", bink2ps5_stub_06);
uint64_t APS5_VABI bink2ps5_stub_06(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "ivBHf-nRkgY");
    return 0;
}

APS5_EXPORT("IXgFcanS2v4", bink2ps5_stub_07);
uint64_t APS5_VABI bink2ps5_stub_07(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "IXgFcanS2v4");
    return 0;
}

APS5_EXPORT("jRGnyN0asN0", bink2ps5_stub_08);
uint64_t APS5_VABI bink2ps5_stub_08(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "jRGnyN0asN0");
    return 0;
}

APS5_EXPORT("kGq53EISh-0", bink2ps5_stub_09);
uint64_t APS5_VABI bink2ps5_stub_09(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "kGq53EISh-0");
    return 0;
}

APS5_EXPORT("m03EhZlYZew", bink2ps5_stub_10);
uint64_t APS5_VABI bink2ps5_stub_10(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "m03EhZlYZew");
    return 0;
}

APS5_EXPORT("MSSY9bBRxHY", bink2ps5_stub_11);
uint64_t APS5_VABI bink2ps5_stub_11(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "MSSY9bBRxHY");
    return 0;
}

APS5_EXPORT("NYHnUggK-hA", bink2ps5_stub_12);
uint64_t APS5_VABI bink2ps5_stub_12(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "NYHnUggK-hA");
    return 0;
}

APS5_EXPORT("pO5SeoJbRt0", bink2ps5_stub_13);
uint64_t APS5_VABI bink2ps5_stub_13(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "pO5SeoJbRt0");
    return 0;
}

APS5_EXPORT("TJyfOXNz7a4", bink2ps5_stub_14);
uint64_t APS5_VABI bink2ps5_stub_14(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "TJyfOXNz7a4");
    return 0;
}

APS5_EXPORT("UOUpNbLHx2g", bink2ps5_stub_15);
uint64_t APS5_VABI bink2ps5_stub_15(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "UOUpNbLHx2g");
    return 0;
}

APS5_EXPORT("wse4gunZeJU", bink2ps5_stub_16);
uint64_t APS5_VABI bink2ps5_stub_16(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "wse4gunZeJU");
    return 0;
}

APS5_EXPORT("yfz-zjkJDkE", bink2ps5_stub_17);
uint64_t APS5_VABI bink2ps5_stub_17(void) {
    APS5_LOG_ERR("Bink2PS5 stub %s called", "yfz-zjkJDkE");
    return 0;
}

}
