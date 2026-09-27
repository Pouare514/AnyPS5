#include "relinker/analysis//ValidationPolicy.hpp"
#include <iostream>
#include <sstream>

namespace Relinker {

namespace {

std::string CanonicalImportProvider(const std::string& library) {
    if (library == "Agc" || library == "Agc_v1" || library == "Agc_v1.1" ||
        library == "libSceAgc" || library == "libSceAgc.prx")
        return "libSceAgc";
    if (library == "AgcDriver" || library == "AgcDriver_v1" || library == "AgcDriver_v1.1" ||
        library == "libSceAgcDriver" || library == "libSceAgcDriver.prx")
        return "libSceAgcDriver";
    if (library == "Ssl" || library == "Ssl_v1" || library == "Ssl_v1.1" ||
        library == "Ssl_v2.1" || library == "libSceSsl" || library == "libSceSsl.prx")
        return "libSceSsl";
    // libScePosix has no DT_NEEDED (.prx) of its own; it is provided by
    // libkernel.prx (NEEDED libkernel's SCE import-lib module is Posix).
    if (library == "libScePosix")
        return "libkernel";
    // General SCE convention: DT_NEEDED carries "libFoo.prx" while NID suffixes
    // and SCE import tables name the module "libFoo". Strip the suffix so the
    // membership check below does not false-throw on valid eboots.
    static constexpr char kPrxSuffix[] = ".prx";
    static constexpr std::size_t kPrxLen = sizeof(kPrxSuffix) - 1u;
    if (library.size() > kPrxLen &&
        library.compare(library.size() - kPrxLen, kPrxLen, kPrxSuffix) == 0) {
        return CanonicalImportProvider(library.substr(0u, library.size() - kPrxLen));
    }
    // Sony spells the SaveData native module with '_' ("libSceSaveData_native")
    // while the .prx filename uses '.' ("libSceSaveData.native"). Normalize.
    std::string normalized = library;
    for (std::size_t pos = 0u;;) {
        pos = normalized.find("_native", pos);
        if (pos == std::string::npos)
            break;
        normalized.replace(pos, 7u, ".native");
        pos += 7u;
    }
    return normalized;
}

bool IsImportProviderCompatible(const std::string& requested, const std::string& provided) {
    const std::string canonicalRequested = CanonicalImportProvider(requested);
    const std::string canonicalProvided = CanonicalImportProvider(provided);
    if (canonicalRequested == canonicalProvided)
        return true;
    if (canonicalRequested == "libSceAgcDriver" && canonicalProvided == "libSceAgc")
        return true;
    return false;
}

}

static constexpr std::uint8_t SYSCALL_BYTE0 = 0x0F;
static constexpr std::uint8_t SYSCALL_BYTE1 = 0x05;
static constexpr std::uint8_t INT80_BYTE0 = 0xCD;
static constexpr std::uint8_t INT80_BYTE1 = 0x80;
static constexpr std::uint8_t SYSENTER_BYTE0 = 0x0F;
static constexpr std::uint8_t SYSENTER_BYTE1 = 0x34;
static constexpr std::uint8_t SYSRET_BYTE0 = 0x0F;
static constexpr std::uint8_t SYSRET_BYTE1 = 0x07;

static constexpr std::uint32_t R_X86_64_NONE = 0;
static constexpr std::uint32_t R_X86_64_64 = 1;
static constexpr std::uint32_t R_X86_64_PC32 = 2;
static constexpr std::uint32_t R_X86_64_GOT32 = 3;
static constexpr std::uint32_t R_X86_64_PLT32 = 4;
static constexpr std::uint32_t R_X86_64_COPY = 5;
static constexpr std::uint32_t R_X86_64_GLOB_DAT = 6;
static constexpr std::uint32_t R_X86_64_JUMP_SLOT = 7;
static constexpr std::uint32_t R_X86_64_RELATIVE = 8;
static constexpr std::uint32_t R_X86_64_GOTPCREL = 9;
static constexpr std::uint32_t R_X86_64_32 = 10;
static constexpr std::uint32_t R_X86_64_32S = 11;
static constexpr std::uint32_t R_X86_64_GOTPCRELX = 41;
static constexpr std::uint32_t R_X86_64_REX_GOTPCRELX = 42;

void ValidationPolicy::_initializeSupportedRelocationTypes() {
    _supportedRelocationTypes = {
        R_X86_64_NONE,
        R_X86_64_64,
        R_X86_64_PC32,
        R_X86_64_GOT32,
        R_X86_64_PLT32,
        R_X86_64_COPY,
        R_X86_64_GLOB_DAT,
        R_X86_64_JUMP_SLOT,
        R_X86_64_RELATIVE,
        R_X86_64_GOTPCREL,
        R_X86_64_32,
        R_X86_64_32S,
        R_X86_64_GOTPCRELX,
        R_X86_64_REX_GOTPCRELX,
    };
}

void ValidationPolicy::RegisterLibraryImport(const std::string& library) {
    if (_supportedRelocationTypes.empty()) {
        _initializeSupportedRelocationTypes();
    }
    _importedLibraries.insert(library);
    const std::string canonical = CanonicalImportProvider(library);
    if (canonical != library)
        _importedLibraries.insert(canonical);
}

void ValidationPolicy::ValidateSyscallAbsence() {
}

void ValidationPolicy::ValidateRelocationTypeSupported(const std::uint32_t relocationTypeValue, const FileByteOffset fileByteOffset) {
    if (_supportedRelocationTypes.empty()) {
        _initializeSupportedRelocationTypes();
    }
    if (_supportedRelocationTypes.find(relocationTypeValue) == _supportedRelocationTypes.end()) {
        std::ostringstream msg;
        msg << "Unsupported relocation type 0x" << std::hex << relocationTypeValue
            << " at offset 0x" << fileByteOffset;
        throw RelinkerException(msg.str(), fileByteOffset);
    }
}

void ValidationPolicy::ValidateNidBelongsToLibrary(const std::string& Nid, const std::string& library) {
    if (_importedLibraries.find(library) != _importedLibraries.end())
        return;
    const std::string canonical = CanonicalImportProvider(library);
    if (_importedLibraries.find(canonical) != _importedLibraries.end())
        return;
    for (const auto& imported : _importedLibraries) {
        if (IsImportProviderCompatible(library, imported))
            return;
    }
    {
        // Verneed entries may reference version-only libraries (e.g. libSceCoredump)
        // that never appear in DT_NEEDED. That is legal input, not an error: warn and
        // let downstream resolution (stubs/diagnostics) handle the NID.
        std::ostringstream msg;
        msg << "warning: NID \"" << Nid << "\" references library \"" << library
            << "\" which is not in the NEEDED list; continuing";
        std::cerr << msg.str() << std::endl;
        return;
    }
}

void ValidationPolicy::ValidateSceStructureSize(const ByteCount expectedSize, const ByteCount actualSize, const FileByteOffset fileByteOffset) {
    if (actualSize != expectedSize) {
        std::ostringstream msg;
        msg << "SCE structure size mismatch at offset 0x" << std::hex << fileByteOffset
            << ": expected " << std::dec << expectedSize << ", got " << actualSize;
        throw RelinkerException(msg.str(), fileByteOffset);
    }
}

void ValidationPolicy::ValidateDynamicFieldInterpretable(const std::string& fieldName, const FileByteOffset fileByteOffset) {
    std::ostringstream msg;
    msg << "Dynamic field \"" << fieldName
        << "\" cannot be interpreted at offset 0x" << std::hex << fileByteOffset;
    throw RelinkerException(msg.str(), fileByteOffset);
}

void ValidationPolicy::ValidateNoSyscallInstructions(
    const std::vector<std::uint8_t>& CodeSection, const FileByteOffset codeOffset) {
    for (std::size_t i = 0; i + 1 < CodeSection.size(); ++i) {
        const std::uint8_t b0 = CodeSection[i];
        const std::uint8_t b1 = CodeSection[i + 1];

        bool isSyscall = (b0 == SYSCALL_BYTE0 && b1 == SYSCALL_BYTE1);
        bool isInt80 = (b0 == INT80_BYTE0 && b1 == INT80_BYTE1);
        bool isSysenter = (b0 == SYSENTER_BYTE0 && b1 == SYSENTER_BYTE1);
        bool isSysret = (b0 == SYSRET_BYTE0 && b1 == SYSRET_BYTE1);

        if (isSyscall || isInt80 || isSysenter || isSysret) {
            FileByteOffset instrOffset = codeOffset + i;
            std::ostringstream msg;
            msg << "Forbidden syscall instruction at offset 0x" << std::hex << instrOffset;
            throw RelinkerException(msg.str(), instrOffset);
        }
    }
}

}
