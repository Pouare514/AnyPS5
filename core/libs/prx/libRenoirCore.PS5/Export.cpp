#include <cstdint>
#include <cstdio>

#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

// libRenoirCore.PS5 replacement (game-custom renderer lib, e.g. Minecraft Legends).
//
// The 23 NIDs below are imported by libcohtml.Prospero.prx (lib libRenoirCore.PS5_v1)
// and their clear names are unknown (obfuscated NIDs), so each export is exposed
// with its raw NID string via APS5_EXPORT (alias "<NID>_nid_no_patch_cut", which the
// nid_patcher strips to the literal NID instead of hashing it).
//
// Stub policy mirrors KytyPS5's UnresolvedImportStub (log the call, return 0) rather
// than the local NotImplemented_nid_no_patch helper (which throws): Renoir calls may
// happen per-frame and must not unwind through the guest.

extern "C" {

APS5_EXPORT("0DsDTr534gc", renoirStub00);
uint64_t APS5_VABI renoirStub00(void) {
    APS5_LOG_OUT("stub NID=%s called", "0DsDTr534gc");
    return 0;
}

APS5_EXPORT("2T56HgbPW08", renoirStub01);
uint64_t APS5_VABI renoirStub01(void) {
    APS5_LOG_OUT("stub NID=%s called", "2T56HgbPW08");
    return 0;
}

APS5_EXPORT("3HnCozh8Az8", renoirStub02);
uint64_t APS5_VABI renoirStub02(void) {
    APS5_LOG_OUT("stub NID=%s called", "3HnCozh8Az8");
    return 0;
}

APS5_EXPORT("5Ysce+s-CWU", renoirStub03);
uint64_t APS5_VABI renoirStub03(void) {
    APS5_LOG_OUT("stub NID=%s called", "5Ysce+s-CWU");
    return 0;
}

APS5_EXPORT("AdlSqA4lGS8", renoirStub04);
uint64_t APS5_VABI renoirStub04(void) {
    APS5_LOG_OUT("stub NID=%s called", "AdlSqA4lGS8");
    return 0;
}

APS5_EXPORT("bC37hk7rHqs", renoirStub05);
uint64_t APS5_VABI renoirStub05(void) {
    APS5_LOG_OUT("stub NID=%s called", "bC37hk7rHqs");
    return 0;
}

APS5_EXPORT("dfPK9R6M0qM", renoirStub06);
uint64_t APS5_VABI renoirStub06(void) {
    APS5_LOG_OUT("stub NID=%s called", "dfPK9R6M0qM");
    return 0;
}

APS5_EXPORT("fZiBbVa2ZDU", renoirStub07);
uint64_t APS5_VABI renoirStub07(void) {
    APS5_LOG_OUT("stub NID=%s called", "fZiBbVa2ZDU");
    return 0;
}

APS5_EXPORT("hkWBH8-WfW4", renoirStub08);
uint64_t APS5_VABI renoirStub08(void) {
    APS5_LOG_OUT("stub NID=%s called", "hkWBH8-WfW4");
    return 0;
}

APS5_EXPORT("hUxvGdi64Io", renoirStub09);
uint64_t APS5_VABI renoirStub09(void) {
    APS5_LOG_OUT("stub NID=%s called", "hUxvGdi64Io");
    return 0;
}

APS5_EXPORT("NxqsZxGisrA", renoirStub10);
uint64_t APS5_VABI renoirStub10(void) {
    APS5_LOG_OUT("stub NID=%s called", "NxqsZxGisrA");
    return 0;
}

APS5_EXPORT("OgZVIkaNmb0", renoirStub11);
uint64_t APS5_VABI renoirStub11(void) {
    APS5_LOG_OUT("stub NID=%s called", "OgZVIkaNmb0");
    return 0;
}

APS5_EXPORT("ojLQdRUnVgA", renoirStub12);
uint64_t APS5_VABI renoirStub12(void) {
    APS5_LOG_OUT("stub NID=%s called", "ojLQdRUnVgA");
    return 0;
}

APS5_EXPORT("s0qDfgsAgVg", renoirStub13);
uint64_t APS5_VABI renoirStub13(void) {
    APS5_LOG_OUT("stub NID=%s called", "s0qDfgsAgVg");
    return 0;
}

APS5_EXPORT("sgrd-r1Nw60", renoirStub14);
uint64_t APS5_VABI renoirStub14(void) {
    APS5_LOG_OUT("stub NID=%s called", "sgrd-r1Nw60");
    return 0;
}

APS5_EXPORT("sjTRb8YD7dQ", renoirStub15);
uint64_t APS5_VABI renoirStub15(void) {
    APS5_LOG_OUT("stub NID=%s called", "sjTRb8YD7dQ");
    return 0;
}

APS5_EXPORT("tG86XGc2VtE", renoirStub16);
uint64_t APS5_VABI renoirStub16(void) {
    APS5_LOG_OUT("stub NID=%s called", "tG86XGc2VtE");
    return 0;
}

APS5_EXPORT("umlGxOcNzgc", renoirStub17);
uint64_t APS5_VABI renoirStub17(void) {
    APS5_LOG_OUT("stub NID=%s called", "umlGxOcNzgc");
    return 0;
}

APS5_EXPORT("vPxA9OFW0os", renoirStub18);
uint64_t APS5_VABI renoirStub18(void) {
    APS5_LOG_OUT("stub NID=%s called", "vPxA9OFW0os");
    return 0;
}

APS5_EXPORT("X2qEBlOLkNo", renoirStub19);
uint64_t APS5_VABI renoirStub19(void) {
    APS5_LOG_OUT("stub NID=%s called", "X2qEBlOLkNo");
    return 0;
}

APS5_EXPORT("ynh4E+BIaqY", renoirStub20);
uint64_t APS5_VABI renoirStub20(void) {
    APS5_LOG_OUT("stub NID=%s called", "ynh4E+BIaqY");
    return 0;
}

APS5_EXPORT("YXe6ABbHZPs", renoirStub21);
uint64_t APS5_VABI renoirStub21(void) {
    APS5_LOG_OUT("stub NID=%s called", "YXe6ABbHZPs");
    return 0;
}

APS5_EXPORT("z0jsXZVz75I", renoirStub22);
uint64_t APS5_VABI renoirStub22(void) {
    APS5_LOG_OUT("stub NID=%s called", "z0jsXZVz75I");
    return 0;
}

}
