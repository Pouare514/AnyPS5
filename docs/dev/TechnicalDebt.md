# Project technical debt

### Build

- Building on Windows requires a specific version of mingw - MinGW-w64 GCC 15.2.0 (`winlibs-gcc15`, `x86_64-ucrt-posix-seh`)
- Even compiled prx libraries on Windows require nearby (static linking of these dependencies causes conflicts):
  - libgcc_s_seh-1.dll
  - libstdc++-6.dll
  - libwinpthread-1.dll

### Silent stubs

Throughout the project, every function at every stage either **does exactly what it's supposed to or throws an exception**. Everywhere... except:
- [libSceSaveDataDialog.native](../../core/libs/prx/libSceSaveDataDialog.native/Export.cpp)
- [libSceCommonDialog](../../core/libs/prx/libSceCommonDialog/Export.cpp)
- [libSceHmd](../../core/libs/prx/libSceHmd/Export.cpp) implements only the disconnected-headset path: initialization succeeds, device queries report `NotDetected`, and opening a device returns `DeviceDisconnected`. Headset support, tracking, rendering and additional HMD exports are not implemented; SDK-level ABI compatibility and in-game behaviour remain unverified.
- [libSceNpCommerce](../../core/libs/prx/libSceNpCommerce/Export.cpp) - the PS Store icon show/hide calls do nothing
- The shader recompiler [skips baryctric coordinates](../../core/shader/recompiler/Recompiler.cpp) (is not even passed to SpirvTargetOptions at row 212).

### Unknown function info

- [AMPR WaitOnAddress / WaitOnCounter](../../core/libs/prx/libkernel/Apr/src/Apr.cpp) (libSceAmpr) - compare encoding assumed to be the WAIT_REG_MEM one (0 always, 1 <, 2 <=, 3 ==, 4 !=, 5 >=, 6 >)
- [sceAgcSetSubmitMode](../../core/libs/prx/libSceAgc/Misc/src/Suspend.cpp) (libSceAgc) - mode values unknown; only 0 is accepted
- [zARR5aCmkoY](../../core/libs/prx/libSceAgc/DcbFlow/src/Control.cpp) (libSceAgc) - unknown name, signature
- [qj7QZpgr9Uw](../../core/libs/prx/libSceAgc/DcbState/src/ContextState.cpp) (libSceAgc) - unknown name
- [fd5Bp5tGTgo](../../core/libs/prx/libSceAgc/Misc/src/ShaderFusion.cpp) (libSceAgc) - unknown name
- [dolOmWH+huQ](../../core/libs/prx/libSceAgc/Misc/src/ShaderFusion.cpp) (libSceAgc) - unknown name
- [V++UgBtQhn0](../../core/libs/prx/libSceAgc/Misc/src/PacketInfo.cpp) (libSceAgc) - unknown name
- [gQkqkLttcpw](../../core/libs/prx/libSceAgc/Acb/src/Control.cpp) (libSceAgc) - unknown name, signature
- [sceKernelInternalMemoryGetModuleSegmentInfo](../../core/libs/prx/libkernel/Module/src/Module.cpp) (libkernel) - unknown signature
- [sceLibcInternalBacktraceForGame](../../core/libs/prx/libc/src/HeapDiagnostics.cpp) (libSceLibcInternal, implemented in libc) - unknown signature
- [sceLibcInternalHeapErrorReportForGame](../../core/libs/prx/libc/src/HeapDiagnostics.cpp) (libSceLibcInternal, implemented in libc) - unknown signature
- [__progname](../../core/libs/prx/libkernel/System/src/Process.cpp) (libkernel) - unknown data export
- [sceSslClose](../../core/libs/prx/libSceSsl/Export.cpp) (libSceSsl) - unknown signature
- [sceSslGetSerialNumber](../../core/libs/prx/libSceSsl/Export.cpp) (libSceSsl) - unknown signature
- [libREDGalaxy64](../../core/libs/prx/libREDGalaxy64/Export.cpp) (game-bundled Galaxy wrapper) - 1icN9EO2WSg, SA7aCTRE5zg, ZVPOpEqpUvo, 4trn+7BssaY, DQAUzFVaGl8, GKomji4WVHw, 36lort-kFp4, NgDLhCc6F7Y, huJ+beHcWBE, sxiZFliOEDU, 49Bdn1OOJ90, 8ahh9DUqZLY, Ard4nEfw5Vs, AzO2fwK+MUs, HKIwQPq45o4, IXRuom5imkM, LjDqfncevos, Onwg0aGsFhM, oSyqfoYcK40, qoS0gqW7RxI, SOtPNNwajSw, udfpd9vbE+w, vuGujcpcUEk, VXUTdiaHbD4, w5-trCio4bQ, YpZjREtZb20, gPJN5c9XCec, z4SpWc89no8, zdzopKdBeU4: unknown names, signatures
- [libc Cyberpunk unknowns](../../core/libs/prx/libc/Export.cpp) (libc) - u2tMGOLaqnE, 2gbcltk3swE, DiGVep5yB5w, Ye20uNnlglA, 5Lf51jvohTQ, 79s2tnYQI6I, 7yMFgcS8EPA, Cj+Fw5q1tUo, CyXs2l-1kNA, H+8UBOwfScI, JhVR7D4Ax6Y, SreZybSRWpU, VsP3daJgmVA, YaHc3GS7y7g, gTuXQwP9rrs, iS4aWbUonl0, vEaqE-7IZYc, DQbtGaBKlaw, rWSuTWY2JN0, tfNbpqL3D0M: unknown names, signatures (qdGFBoLVNKI is quick_exit, implemented separately)
- [libkernel Cyberpunk unknowns](../../core/libs/prx/libkernel/Unimplemented.cpp) (libkernel) - vNe1w4diLCs, teiItL2boFw, 0D4-FVvEikw (canonical libScePosix, unresolved at load): unknown names, signatures
- [libSceNpCppWebApi Cyberpunk unknowns](../../core/libs/prx/libSceNpCppWebApi/Export.cpp) (libSceNpCppWebApi) - +gSMGXHgrPU, -XNfwZTFkWw, 0f5+eBcgB14, 1iq5Jtw4jVs, 1wpCXuFzH10, 4AFThLW28xQ, 8-9Y4oS+OuE, 8dWkWjo5EeE, BHvfIGdarBQ, Fxxr5lYBfl4, M1LnjRscx-w, PsX657reZeo, RvH9AZ5rCkQ, SR3BKonD8yk, TSoQhYKyq5g, TgsJjQqp+2k, UHQxE3HhTXA, ZN-glpk0Rug, ZQzXwokCnUE, ZwjmbSr4Cxc, b7leY-LNVnI, c3J9K9XbQqE, cSLbQUQO2Ns, cqOxjL0ZfFA, d1eEjWR60wk, epwr+cBCIFs, eruduJ0KrT0, fG06a38iao8, jfpSx14AeLc, kBxwE4YfbIM, kz2Z38yTLq0, lB8driFKaoU, mCsV7izOkjY, n1yCLMxpseQ, nL9vSVq-29s, nmz5JYKcMfY, oHRl7a+zdMU, pSpO3hPNv64, q4UFICay6Hk, qiD2PU2Jstc, tFe964qzGEM, tHaO36kincQ, umm5m+mXiZs, uwhztB49KOQ, w4K4nTYwhVE, wFcm6bgWB9Q, wp8+c84G5Xw, zjqTmP0ST9A, zzO8ZGJ74ng, 12wY179+CE8, 3bjEFf8OhTs, 6FK4IOnANkc, EJAOHYWUki4, FlagLhjAEmc, G3h-NDHnyW4, H0XjNhflED4, IbWx007Acn4, Is+nI7Hq9jU, Kc4x2uy0FFk, OK+Ggpve7B0, P3daBLFFREw, RZcseJ3THfY, RbvteD35X-8, XCtf+23QT0k, a4q15LI1a4E, eT4TQB7OsLA, g2dEqFnhFuw, lPAMjFZEpt8, mBagn+lW-iM, mkWsKEh0h0o, nE0ooeCMRm8, qf2I9BlKXis, xwy1I52dE8o: unknown names, signatures
- [libSceCoredump Cyberpunk unknowns](../../core/libs/prx/libkernel/System/src/Coredump.cpp) (libkernel) - 5nc2gdLNsok, Jrs7UUkGOFo, MEJ7tc7ThwM, Uxqkdta7wEg, dei8oUx6DbU, kK0DUW1Ukgc: unknown names, signatures
- [libSceHttp Cyberpunk unknowns](../../core/libs/prx/libSceHttp/Export.cpp) (libSceHttp) - i9mhafzkEi8, vO4B-42ef-k: unknown names, signatures
- [sceNetUnknown00](../../core/libs/prx/libSceNet/Export.cpp) (libSceNet) - TCkRD0DWNLg: unknown name, signature
- [sceContentExportUnknown00](../../core/libs/prx/libSceContentExport/Export.cpp) (libSceContentExport) - AOWqIYsgVHs: unknown name, signature
- [sceAppContentUnknown00](../../core/libs/prx/libSceAppContent/Export.cpp) (libSceAppContent) - 7gxh+5QubhY: unknown name, signature
- [sceJson2Unknown00](../../core/libs/prx/libSceJson2/Export.cpp) (libSceJson2) - 6i18OJSvFWk: unknown name, signature
- [sceMsgDialogUnknown00](../../core/libs/prx/libSceMsgDialog.native/Export.cpp) (libSceMsgDialog.native) - CWVW78Qc3fI: unknown name, signature
- [libSceSaveData.native Cyberpunk unknowns](../../core/libs/prx/libSceSaveData.native/Export.cpp) (libSceSaveData.native) - RjMlsR8EXrw, X4MYzukPc3g: unknown names, signatures
- [scePlayGoDialogUnknown00](../../core/libs/prx/libScePlayGoDialog/Export.cpp) (libScePlayGoDialog) - NOAMxY2EGS0: unknown name, signature

### Functional

- [Shader recompilation](../../core/shader/recompiler/Recompiler.cpp) currently occurs right before it was transferred to Vulkan with caching, but should be moved to the [relinker](../../core/relinker/main.cpp) stage. For this purpose, [shader/recompiler](../../core/shader/recompiler) was written completely independently from [libs/prx](../../core/libs/prx).
- The executable file that [relinker](../../core/relinker/elfpatcher/src/windows/WindowsPeWriter.cpp) generates opens the console when launched, which is inconvenient for playability.
- [libSceJpegEnc](../../core/libs/prx/libSceJpegEnc/Export.cpp) encodes 4:2:2 sampling requests with 4:2:0 chroma subsampling, and grayscale requests as a 3-component JPEG with neutral chroma instead of a single-component one: the [JPEG encoder](../../core/Decoder/Jpeg/src/Jpeg.cpp) (stb) only produces 3-component 4:2:0 and 4:4:4 images.
- [Relinker](../../core/relinker/elfpatcher/src) doesn't add an icon to the generated executable. This should be done without adding dependencies (only standard).
- `--to-intel` does not lower RDPRU/MCOMMIT; the [matcher](../../core/relinker/codegen/src/x86/Amd64OnlyInstructionMatcher.cpp) fails the relink instead. The SHA-1 instructions of SHA-NI (SHA1RNDS4, SHA1NEXTE, SHA1MSG1, SHA1MSG2) are not substituted, and SHA-256 instructions with a memory operand fail the relink.
- The length-changing path of the [instruction rewriter](../../core/relinker/codegen/src/x86/X64InstructionRewriter.cpp) is not used by the [converter](../../core/relinker/codegen/src/Amd64OnlyConverter.cpp): it does not adjust VEX/0F38/0F3A RIP-relative operands, data-to-code references (relocations, FDEs, jump tables) or segment sizes, so every substitution keeps the instruction length.
- The Linux placement of the `--to-intel` stubs in the [ELF patcher](../../core/relinker/elfpatcher/src/linux/LinuxElfPatcher.cpp) is covered only by a synthetic test.
- The libc [SSE4a trap emulation](../../core/libs/prx/libc/src/specifics/windows/Sse4aEmulation.hpp) on Windows is superseded by `--to-intel` and remains only until the relinked title has been verified without it.
- `--to-intel` guest module trampolines are covered only by a synthetic relinker test; no game title has been verified with them on Linux or Windows.
- [sceKeyboardGetKey2Char](../../core/libs/prx/libSceKeyboard/src/keyboard_impl.cpp) (libSceKeyboard) translates only the 101-key (US) arrangement and throws for the 106-key (Japanese) one; Ctrl and Alt do not change the character.
- [libSceAudiodec](../../core/libs/prx/libSceAudiodec/Export.cpp) throws for the 24-bit PCM word size (`iBwPcm` 0): its sample layout is unknown. ATRAC9 decoding is covered only by configuration and error tests, as no ATRAC9 encoder is available for a fixture.
- [libScePlayerInvitationDialog](../../core/libs/prx/libScePlayerInvitationDialog/libScePlayerInvitationDialog.cpp) simulates dialog completion without displaying UI or sending invitations; its parameter ABI remains unverified.
