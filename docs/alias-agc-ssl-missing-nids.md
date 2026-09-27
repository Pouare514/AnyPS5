# Alias Agc/Ssl — 26 missing NID bodies (out of scope)

Round 1 recomputed NIDs (SHA1 + custom base64, Sony suffix key) against the
Legends imports in `docs/legends-unresolved-nids.json`:

- `Agc_v1` (`Agc_v1.1`): 60/69 match (57 computed + 3 raw `APS5_EXPORT`), 9 missing.
- `AgcDriver_v1` (`AgcDriver_v1.1`): 5/17 match, 12 missing.
- `Ssl_v1` (`Ssl_v2.1`): 0/5 match, 5 missing.
- Total: 65/91 fixed, 26 missing.

Do NOT implement the 26 bodies in this round — they need Sony DB research
(exact symbol names behind the hashes + real GPU/SSL semantics). This note
lists them so a later round can pick them up. Source of truth for the hashes:
`docs/legends-unresolved-nids.json` (eboot.bin importer).

## Agc_v1 — 9 missing (of 69)

Implemented: 57 via `ComputeNid` + 3 raw `APS5_EXPORT`:
`sceAgcAcb_gQkqkLttcpw` (`gQkqkLttcpw`), `sceAgcDcbA_zARR5aCmkoY`
(`zARR5aCmkoY`), `sceAgcCreateInterpolantMappingVsPs` (`k0E7vkgqAuE`).

Missing (Legends `Agc_v1` NIDs with no host body; likely user-mode GPU
command-buffer / shader / state builders — exact names need Sony DB):

- `7Wa3aeJgeVU`
- `AAeX-U5-P3M`
- `FcgdDM3MB+k`
- `FneFypEDRgY`
- `hFQ9pUxoLQ4`
- `opR1JeJZCBU`
- `OQTgEXyihvA`
- `rP5xLdOf26k`
- `rVOmPz2RBlg`

## AgcDriver_v1 — 12 missing (of 17)

Implemented (5, all trace/capture/validation): `+TN0oRTBxJQ`
(`sceAgcDriverIsTraceInProgress`), `FOwvmNlFLjM`
(`sceAgcDriverRequestCaptureStop`), `qspAL8bgcBY`
(`sceAgcDriverIsSubmitValidationEnabled`), `SAfhzJPcjuk`
(`sceAgcDriverRequestCaptureStart`), `Xq5WmbwPTnQ`
(`sceAgcDriverTriggerCapture`).

Missing (likely core submission / EQ / resource work — exact names need
Sony DB):

- `5l3IfCFJxBs`
- `emP3ckeS2uo`
- `LepGrgk77sM`
- `ls4jfY576lw`
- `M9yBzRKkjPc`
- `mXn+K9E-wOA`
- `n5ElQVYsU1A`
- `NghWEUXp1qM`
- `rI9lNAXPMIw`
- `rJUyMrDdxJg`
- `SCoAN5fYlUM`
- `VOMSpd9+vxU`

## Ssl_v1 (Ssl_v2.1) — 5 missing (of 5)

Existing `core/libs/prx/libSceSsl/Export.cpp` stubs (`sceSslFreeCaCerts`,
`sceSslGetCaCerts`, `sceSslInit_nid_postfix`, `sceSslTerm_nid_postfix`) export
under plain names via `nid_no_patch` — none hash-match the 5 Sony NIDs, so
0/5. The 5 are likely SSL context lifecycle + CA certs + handshake I/O
(exact mapping needs Sony DB; do not guess names from hashes):

- `HJ1n138CQ2g`
- `iNjkt9Poblw`
- `jltWpVKtetg`
- `tuscfitnhEo`
- `zXvd6iNyfgc`
