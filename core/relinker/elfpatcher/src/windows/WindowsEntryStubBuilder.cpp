#include <elfpatcher/windows/WindowsEntryStubBuilder.hpp>
#include <elfpatcher/windows/WindowsStubEmitter.hpp>
#include <elfpatcher/windows/WindowsDependencyStubBuilder.hpp>
#include <elfpatcher/windows/WindowsGuestStartup.hpp>
#include <io/BufferUtils.hpp>
#include <algorithm>
#include <map>
#include <optional>

namespace Elfpatcher::Windows {

namespace {

constexpr std::uint32_t PathCapacity = 32768;
constexpr std::uint32_t ErrorMessageCapacity = 32768;

std::string normalizeRunPath(std::string path) {
    if (path.empty() || std::any_of(path.begin(), path.end(), [](const unsigned char value) { return value == 0 || value >= 128; }))
        throw Domain::RelinkerException("Windows run path must be a nonempty ASCII path");
    std::replace(path.begin(), path.end(), '/', '\\');
    if (path == "$ORIGIN")
        path = ".";
    if (path.starts_with("$ORIGIN\\"))
        path.erase(0, 8);
    if (path.find('$') != std::string::npos)
        throw Domain::RelinkerException("Unsupported variable in Windows run path");
    if (path.back() != '\\')
        path.push_back('\\');
    return path;
}

}

WindowsEntryStub WindowsEntryStubBuilder::Build(const std::uint32_t dataRva, const std::uint32_t entryRva, const WindowsImports& nativeImports, const std::vector<std::string>& libraries, const std::vector<PeImport>& imports, const std::string& runPath, const bool lazyBinding, const bool dependencyDiagnostics, const std::vector<Domain::GuestRuntime>& guestModules) const {
    if (!imports.empty() && libraries.empty())
        throw Domain::RelinkerException("ELF imports have no DT_NEEDED libraries");
    const auto path = normalizeRunPath(runPath);
    const bool absolutePath = path.starts_with("\\\\") || (path.size() >= 3 && path[1] == ':' && path[2] == '\\');
    if (!absolutePath && (path.front() == '\\' || path.find(':') != std::string::npos))
        throw Domain::RelinkerException("Ambiguous Windows run path: " + path);

    WindowsEntryStub result{{".startup", dataRva, SectionRead | SectionWrite | 0x40u, {}}, {".entry", 0, SectionRead | SectionExecute | 0x20u, {}}, {}};
    auto& data = result.Data.Data;

    const auto reserve = [&](const std::size_t size) {
        const auto rva = CheckedRva(dataRva + data.size());
        data.resize(data.size() + size);
        return rva;
    };
    const auto addString = [&](const std::string& value) {
        const auto rva = CheckedRva(dataRva + data.size());
        Io::AppendString(data, value);
        return rva;
    };

    const auto programPath = reserve(PathCapacity);
    const auto modulePath = reserve(PathCapacity);
    const auto handles = reserve(libraries.size() * 8);
    const auto guestFinished = reserve(4);
    const WindowsGuestStartup guestStartup;
    const auto functionTable = reserve(12 * 32);
    const auto unwindRva = CheckedRva(dataRva + data.size());
    data.insert(data.end(), {1, 10, 6, 0, 10, 0xb2, 6, 0xc0, 4, 0x70, 3, 0x60, 2, 0x50, 1, 0x30});

    std::vector<std::uint32_t> libraryPaths;
    std::vector<std::string> libraryNames;
    for (std::size_t index = 0; index < libraries.size(); ++index) {
        auto name = index < guestModules.size() ? libraries[index] : path + libraries[index];
        std::replace(name.begin(), name.end(), '/', '\\');
        if (name.size() + 1 >= PathCapacity)
            throw Domain::RelinkerException("Windows library path exceeds the startup buffer: " + name);
        libraryPaths.push_back(addString(name));
        libraryNames.push_back(std::move(name));
    }

    std::vector<std::uint32_t> symbolNames;
    for (const auto& import : imports)
        symbolNames.push_back(addString(import.Name));

    // Stub policy (emulator-style): every distinct NID gets one stub
    // trampoline so an unresolved import binds to a stub instead of killing
    // the boot. Map each import to its unique-NID index here; resolution
    // itself still happens at runtime via GetProcAddress.
    std::vector<std::size_t> importUid;
    importUid.reserve(imports.size());
    std::vector<std::string> uniqueNames;
    std::vector<std::size_t> firstImport;
    std::map<std::string, std::size_t> uidByName;
    for (std::size_t index = 0; index < imports.size(); ++index) {
        const auto [it, inserted] = uidByName.emplace(imports[index].Name, uniqueNames.size());
        if (inserted) {
            uniqueNames.push_back(imports[index].Name);
            firstImport.push_back(index);
        }
        importUid.push_back(it->second);
    }

    const auto lastError = reserve(4);
    const auto errorDigits = reserve(11);
    const auto errorMessage = reserve(ErrorMessageCapacity);
    const auto loading = addString("Loading PRX: ");
    const auto loaded = addString(" -> OK\n");
    const auto loadFailed = addString(" -> FAILED\n");
    const auto failedModule = addString("Failed to load module: ");
    const auto errorPrefix = addString("GetLastError: ");
    const auto messageSeparator = addString(" - ");
    const auto newline = addString("\n");
    const auto enteringElf = addString("Transferring control to ELF entry point\n");
    std::vector<std::uint32_t> resolvedPaths;
    for (std::size_t index = 0; index < libraries.size(); ++index)
        resolvedPaths.push_back(reserve(PathCapacity));

    const auto diagnosticsOffset = data.size();
    std::vector<std::string> errors = {"FAIL: cannot obtain executable path\n", "FAIL: executable or library path is too long\n", "FAIL: executable path has no directory\n"};
    for (const auto& import : imports)
        errors.push_back("FAIL: unresolved ELF import " + import.Name + "\n");
    std::vector<std::uint32_t> errorRvas;
    for (const auto& error : errors)
        errorRvas.push_back(addString(error));
    if (data.size() <= diagnosticsOffset)
        throw Domain::RelinkerException("Empty startup diagnostics");

    // Stub policy (emulator-style) data. One flag byte per unique NID records
    // whether the call-time log already fired, one counter caps the total
    // number of call-time logs (first 1024, like Kyty's UnresolvedImportStub),
    // and one table maps the stub index to its NID name string for logging.
    // The per-NID bind messages are logged once at startup when GetProcAddress
    // fails, so one run reveals every missing import.
    const auto stubFlags = reserve(uniqueNames.size());
    const auto stubLogCount = reserve(4);
    const auto stubNameTable = CheckedRva(dataRva + data.size());
    for (std::size_t uid = 0; uid < uniqueNames.size(); ++uid)
        Io::AppendU32(data, symbolNames.at(firstImport.at(uid)));
    const auto stubCallPrefix = addString("STUB: called unresolved ELF import ");
    std::vector<std::uint32_t> stubBindMessages;
    for (const auto& name : uniqueNames)
        stubBindMessages.push_back(addString("STUB: unresolved ELF import " + name + "\n"));
    const auto stubUnwindRva = CheckedRva(dataRva + data.size());
    data.insert(data.end(), {1, 4, 1, 0, 4, 0xa1, 0, 0});

    std::optional<WindowsDependencyStubBuilder> dependencyBuilder;
    if (dependencyDiagnostics)
        dependencyBuilder.emplace(result.Data);
    std::vector<std::size_t> dependencyCalls;
    result.Code.Rva = AlignRva(dataRva + data.size());
    WindowsStubEmitter code(result.Code.Rva);

    const auto call = [&](const std::string& name) { code.Rip({0xff, 0x15}, nativeImports.Functions.at(name)); };

    const auto raise = [&](const std::uint32_t status) {
        code.Emit({0xb9});
        code.U32(status);
        code.Emit({0xba, 1, 0, 0, 0, 0x45, 0x31, 0xc0, 0x45, 0x31, 0xc9});
        call("RaiseException");
        code.Emit({0x0f, 0x0b});
    };

    const auto requireDiagnosticSuccess = [&] {
        code.Emit({0x48, 0x85, 0xc0});
        const auto success = code.Branch({0x0f, 0x85});
        raise(0xc0000001u);
        code.PatchBranch(success, code.GetRva());
    };

    const auto writeString = [&](const std::uint32_t stringRva, const bool isError = false) {
        code.Rip({0x48, 0x8d, 0x0d}, stringRva);
        call("lstrlenA");
        code.Emit({0x89, 0x44, 0x24, 0x3c, 0xb9});
        code.U32(isError ? 0xfffffff4u : 0xfffffff5u);
        call("GetStdHandle");
        requireDiagnosticSuccess();
        code.Emit({0x48, 0x83, 0xf8, 0xff});
        const auto validHandle = code.Branch({0x0f, 0x85});
        raise(0xc0000001u);
        code.PatchBranch(validHandle, code.GetRva());
        code.Emit({0x48, 0x89, 0xc1});
        code.Rip({0x48, 0x8d, 0x15}, stringRva);
        code.Emit({0x44, 0x8b, 0x44, 0x24, 0x3c, 0x4c, 0x8d, 0x4c, 0x24, 0x38, 0x48, 0xc7, 0x44, 0x24, 0x20, 0, 0, 0, 0});
        call("WriteFile");
        requireDiagnosticSuccess();
        code.Emit({0x8b, 0x44, 0x24, 0x38, 0x3b, 0x44, 0x24, 0x3c});
        const auto complete = code.Branch({0x0f, 0x84});
        raise(0xc0000001u);
        code.PatchBranch(complete, code.GetRva());
    };

    const auto writeLastError = [&] {
        writeString(errorPrefix, true);
        code.Rip({0x8b, 0x05}, lastError);
        code.Rip({0x48, 0x8d, 0x3d}, errorDigits + 10);
        code.Emit({0xc6, 0x07, 0, 0x41, 0xb8, 10, 0, 0, 0});
        const auto digit = code.GetRva();
        code.Emit({0x31, 0xd2, 0x41, 0xf7, 0xf0, 0x80, 0xc2, 0x30, 0x48, 0xff, 0xcf, 0x88, 0x17, 0x85, 0xc0});
        code.Rip({0x0f, 0x85}, digit);
        code.Emit({0x48, 0x89, 0xfe});
        code.Rip({0x48, 0x8d, 0x0d}, errorDigits + 11);
        code.Emit({0x48, 0x29, 0xf9});
        code.Rip({0x48, 0x8d, 0x3d}, errorDigits);
        code.Emit({0xf3, 0xa4});
        writeString(errorDigits, true);
        writeString(messageSeparator, true);
        code.Emit({0xb9, 0, 0x12, 0, 0, 0x31, 0xd2});
        code.Rip({0x44, 0x8b, 0x05}, lastError);
        code.Emit({0x41, 0xb9, 9, 4, 0, 0});
        code.Rip({0x48, 0x8d, 0x05}, errorMessage);
        code.Emit({0x48, 0x89, 0x44, 0x24, 0x20, 0x48, 0xc7, 0x44, 0x24, 0x28});
        code.U32(ErrorMessageCapacity);
        code.Emit({0x48, 0xc7, 0x44, 0x24, 0x30, 0, 0, 0, 0});
        call("FormatMessageA");
        requireDiagnosticSuccess();
        writeString(errorMessage, true);
    };

    const auto captureLastError = [&] {
        call("GetLastError");
        code.Rip({0x89, 0x05}, lastError);
    };

    const auto fail = [&](const std::size_t error, const std::uint32_t status) {
        writeString(errorRvas.at(error), true);
        raise(status);
    };

    const auto requireNonzero = [&](const std::size_t error, const std::uint32_t status) {
        code.Emit({0x48, 0x85, 0xc0});
        const auto success = code.Branch({0x0f, 0x85});
        captureLastError();
        writeString(errorRvas.at(error), true);
        writeLastError();
        raise(status);
        code.PatchBranch(success, code.GetRva());
    };

    code.Emit({0x53, 0x55, 0x56, 0x57, 0x41, 0x54, 0x48, 0x83, 0xec, 0x60});
    code.Emit({0x31, 0xc9});
    code.Rip({0x48, 0x8d, 0x15}, programPath);
    code.Emit({0x41, 0xb8});
    code.U32(PathCapacity);
    call("GetModuleFileNameA");
    requireNonzero(0, 0xc000000du);
    code.Emit({0x3d});
    code.U32(PathCapacity);
    const auto pathFits = code.Branch({0x0f, 0x82});
    fail(1, 0xc0000106u);
    code.PatchBranch(pathFits, code.GetRva());
    code.Emit({0x89, 0xc1});
    code.Rip({0x48, 0x8d, 0x35}, programPath);
    code.Rip({0x48, 0x8d, 0x3d}, modulePath);
    code.Emit({0xfc, 0xf3, 0xa4, 0x49, 0x89, 0xfc});
    code.Rip({0x48, 0x8d, 0x1d}, modulePath);
    const auto findSeparator = code.GetRva();
    code.Emit({0x49, 0x39, 0xdc});
    const auto hasDirectory = code.Branch({0x0f, 0x85});
    fail(2, 0xc000000du);
    code.PatchBranch(hasDirectory, code.GetRva());
    code.Emit({0x49, 0xff, 0xcc, 0x41, 0x80, 0x3c, 0x24, 0x5c});
    code.Rip({0x0f, 0x85}, findSeparator);
    code.Emit({0x49, 0xff, 0xc4});

    for (std::size_t index = 0; index < libraries.size(); ++index) {
        if (absolutePath && index >= guestModules.size()) {
            code.Rip({0x48, 0x8d, 0x0d}, libraryPaths[index]);
        } else {
            code.Emit({0x4c, 0x89, 0xe0, 0x48, 0x29, 0xd8, 0x48, 0x05});
            code.U32(CheckedRva(libraryNames[index].size() + 1));
            code.Emit({0x48, 0x3d});
            code.U32(PathCapacity);
            const auto fits = code.Branch({0x0f, 0x86});
            fail(1, 0xc0000106u);
            code.PatchBranch(fits, code.GetRva());
            code.Emit({0x4c, 0x89, 0xe7});
            code.Rip({0x48, 0x8d, 0x35}, libraryPaths[index]);
            code.Emit({0xb9});
            code.U32(CheckedRva(libraryNames[index].size() + 1));
            code.Emit({0xf3, 0xa4});
            code.Rip({0x48, 0x8d, 0x0d}, modulePath);
        }
        code.Emit({0x48, 0x89, 0xce});
        call("lstrlenA");
        code.Emit({0x8d, 0x48, 1});
        code.Rip({0x48, 0x8d, 0x3d}, resolvedPaths[index]);
        code.Emit({0xf3, 0xa4});

        if (dependencyDiagnostics) {
            writeString(loading);
            writeString(resolvedPaths[index]);
        }

        code.Rip({0x48, 0x8d, 0x0d}, resolvedPaths[index]);
        code.Emit({0x31, 0xd2, 0x41, 0xb8, 0, 0x11, 0, 0});
        call("LoadLibraryExA");
        code.Emit({0x48, 0x85, 0xc0});
        const auto loadSucceeded = code.Branch({0x0f, 0x85});
        captureLastError();
        writeString(loadFailed, true);
        writeString(failedModule, true);
        writeString(resolvedPaths[index], true);
        writeString(newline, true);
        writeLastError();
        if (dependencyDiagnostics) {
            code.Rip({0x48, 0x8d, 0x0d}, resolvedPaths[index]);
            code.Rip({0x48, 0x8d, 0x15}, programPath);
            dependencyCalls.push_back(code.Branch({0xe8}));
        }
        raise(0xc0000135u);
        code.PatchBranch(loadSucceeded, code.GetRva());
        code.Rip({0x48, 0x89, 0x05}, CheckedRva(handles + index * 8));

        if (dependencyDiagnostics) {
            writeString(loaded);
        }
    }

    std::vector<std::size_t> lazyUnresolvedImports;
    std::vector<std::size_t> tlsResolverAddresses;
    // Forward references to the per-NID stub trampolines emitted after the
    // main bootstrap body: {rel32 field offset, unique-NID index}.
    std::vector<std::pair<std::size_t, std::size_t>> stubLeaFixups;
    // Lazy unresolved imports bound to the same trampolines through the
    // LazyStubs file-patch mechanism (resolved after trampoline emission):
    // {import index, unique-NID index}.
    std::vector<std::pair<std::size_t, std::size_t>> lazyStubFixups;
    for (std::size_t index = 0; index < imports.size(); ++index) {
        if (!guestModules.empty() && guestModules.front().UsePlatformTlsResolver && imports[index].Name == "vNe1w4diLCs") {
            if (imports[index].Addend != 0) throw Domain::RelinkerException("TLS resolver import has an addend");
            tlsResolverAddresses.push_back(code.Branch({0x48, 0x8d, 0x05}));
            guestStartup.WriteImport(code, imports[index], handles);
            continue;
        }
        code.Rip({0x48, 0x8d, 0x1d}, handles);
        code.Rip({0x48, 0x8d, 0x35}, symbolNames[index]);
        code.Emit({0xbd});
        code.U32(CheckedRva(libraries.size()));
        const auto search = code.GetRva();
        code.Emit({0x48, 0x8b, 0x0b, 0x48, 0x89, 0xf2});
        call("GetProcAddress");
        code.Emit({0x48, 0x85, 0xc0});
        const auto resolved = code.Branch({0x0f, 0x85});
        code.Emit({0x48, 0x83, 0xc3, 8, 0xff, 0xcd});
        code.Rip({0x0f, 0x85}, search);
        // Stub policy: an import missing from every library binds its GOT slot
        // to this NID's shared ret-0 trampoline (logged once at bind and once
        // at first call, capped) instead of failing the boot, so one run
        // reveals every missing NID. The R_X86_64_64 nonzero-addend corner
        // binds GOT = stub address (addend skipped): the stub address stands
        // for the symbol value itself.
        if (lazyBinding) {
            // Deferred through the LazyStubs file-patch mechanism, bound to
            // the trampolines after their emission below.
            lazyUnresolvedImports.push_back(index);
            const auto luid = importUid.at(index);
            if (firstImport.at(luid) == index)
                writeString(stubBindMessages.at(luid));
            lazyStubFixups.emplace_back(index, luid);
            const auto skipGotWrite = code.Branch({0xe9});
            code.PatchBranch(resolved, code.GetRva());
            if (imports[index].Addend != 0) {
                code.Emit({0x48, 0xba});
                code.U64(imports[index].Addend);
                code.Emit({0x48, 0x01, 0xd0});
            }
            guestStartup.WriteImport(code, imports[index], handles);
            code.PatchBranch(skipGotWrite, code.GetRva());
            continue;
        }
        const auto uid = importUid.at(index);
        if (firstImport.at(uid) == index)
            writeString(stubBindMessages.at(uid));
        stubLeaFixups.emplace_back(code.Branch({0x48, 0x8d, 0x05}), uid);
        code.Rip({0x48, 0x89, 0x05}, imports[index].TargetRva);
        const auto stubDone = code.Branch({0xe9});
        code.PatchBranch(resolved, code.GetRva());
        if (imports[index].RelocationType == 16) code.Emit({0x48, 0x8b, 0x00});
        if (imports[index].RelocationType == 17) code.Emit({0x48, 0x8b, 0x40, 8});
        if (imports[index].Addend != 0) {
            code.Emit({0x48, 0xba});
            code.U64(imports[index].Addend);
            code.Emit({0x48, 0x01, 0xd0});
        }
        code.Rip({0x48, 0x89, 0x05}, imports[index].TargetRva);
        guestStartup.WriteImport(code, imports[index], handles);
        code.PatchBranch(stubDone, code.GetRva());
    }

    guestStartup.Initialize(code, guestModules, handles);
    writeString(enteringElf);
    code.Emit({0x48, 0xc7, 0x44, 0x24, 0x40, 1, 0, 0, 0});
    code.Rip({0x48, 0x8d, 0x05}, programPath);
    code.Emit({0x48, 0x89, 0x44, 0x24, 0x48, 0x31, 0xc0, 0x48, 0x89, 0x44, 0x24, 0x50, 0x48, 0x89, 0x44, 0x24, 0x58, 0x48, 0x8d, 0x7c, 0x24, 0x40});
    const auto exitCallback = code.Branch({0x48, 0x8d, 0x35});
    code.Rip({0xe8}, entryRva);
    code.Emit({0x89, 0x44, 0x24, 0x58});
    guestStartup.Finalize(code, guestModules, handles, guestFinished);
    code.Emit({0x8b, 0x44, 0x24, 0x58});
    code.Emit({0x89, 0xc1});
    call("ExitProcess");
    code.Emit({0x0f, 0x0b});

    const auto functionEnd = code.GetRva();
    code.PatchBranch(exitCallback, functionEnd);
    code.Emit({0x48, 0x83, 0xec, 0x28});
    guestStartup.Finalize(code, guestModules, handles, guestFinished);
    code.Emit({0x48, 0x83, 0xc4, 0x28});
    code.Emit({0xc3});
    const auto tlsResolver = guestStartup.EmitTlsResolver(code);
    for (const auto offset : tlsResolverAddresses) code.PatchBranch(offset, tlsResolver);

    // Shared unresolved-import stub (emulator-style, cf. Kyty's
    // UnresolvedImportStub). Entered with the unique-NID index in ecx via a
    // per-NID trampoline. Logs the NID on first call per NID (capped at 1024
    // total logs) and returns 0. Only volatile registers under both the
    // Windows x64 and System V AMD64 ABIs (rax, rcx, rdx, r8-r11) are touched,
    // so the stub is callable with any signature; xmm0 is also zeroed so
    // floating-point returns read +0.0. Failures while logging are silently
    // skipped: a stub must never kill the game.
    const auto writeStubString = [&](const bool dynamic, const std::uint32_t stringRva) {
        if (dynamic)
            code.Emit({0x48, 0x8b, 0x4c, 0x24, 0x48});
        else
            code.Rip({0x48, 0x8d, 0x0d}, stringRva);
        call("lstrlenA");
        code.Emit({0x48, 0x85, 0xc0});
        const auto empty = code.Branch({0x0f, 0x84});
        code.Emit({0x89, 0x44, 0x24, 0x3c, 0xb9});
        code.U32(0xfffffff5u);
        call("GetStdHandle");
        code.Emit({0x48, 0x85, 0xc0});
        const auto noHandle = code.Branch({0x0f, 0x84});
        code.Emit({0x48, 0x83, 0xf8, 0xff});
        const auto invalidHandle = code.Branch({0x0f, 0x84});
        code.Emit({0x48, 0x89, 0xc1});
        if (dynamic)
            code.Emit({0x48, 0x8b, 0x54, 0x24, 0x48});
        else
            code.Rip({0x48, 0x8d, 0x15}, stringRva);
        code.Emit({0x44, 0x8b, 0x44, 0x24, 0x3c, 0x4c, 0x8d, 0x4c, 0x24, 0x38, 0x48, 0xc7, 0x44, 0x24, 0x20, 0, 0, 0, 0});
        call("WriteFile");
        const auto next = code.GetRva();
        code.PatchBranch(empty, next);
        code.PatchBranch(noHandle, next);
        code.PatchBranch(invalidHandle, next);
    };

    const auto stubLoggerRva = code.GetRva();
    code.Emit({0x48, 0x83, 0xec, 0x58});
    code.Emit({0x89, 0x4c, 0x24, 0x40});
    code.Rip({0x48, 0x8d, 0x15}, stubFlags);
    code.Emit({0x8b, 0x44, 0x24, 0x40});
    code.Emit({0x80, 0x3c, 0x02, 0x00});
    const auto alreadyLogged = code.Branch({0x0f, 0x85});
    code.Emit({0xc6, 0x04, 0x02, 0x01});
    code.Rip({0x8b, 0x05}, stubLogCount);
    code.Emit({0x3d, 0x00, 0x04, 0x00, 0x00});
    const auto logCapped = code.Branch({0x0f, 0x83});
    code.PatchBranch(code.Branch({0xf0, 0xff, 0x05}), stubLogCount);
    code.Rip({0x48, 0x8d, 0x15}, stubNameTable);
    code.Emit({0x8b, 0x44, 0x24, 0x40});
    code.Emit({0x8b, 0x04, 0x82});
    const auto getBase = code.Branch({0xe8});
    const auto popRva = code.GetRva();
    code.PatchBranch(getBase, popRva);
    code.Emit({0x5a});
    code.Emit({0x48, 0x81, 0xea});
    code.U32(popRva);
    code.Emit({0x48, 0x01, 0xc2});
    code.Emit({0x48, 0x89, 0x54, 0x24, 0x48});
    writeStubString(false, stubCallPrefix);
    writeStubString(true, 0);
    writeStubString(false, newline);
    const auto returnZero = code.GetRva();
    code.PatchBranch(alreadyLogged, returnZero);
    code.PatchBranch(logCapped, returnZero);
    code.Emit({0x0f, 0x57, 0xc0, 0x31, 0xc0, 0x48, 0x83, 0xc4, 0x58, 0xc3});
    const auto stubLoggerEnd = code.GetRva();

    // One trampoline per unique NID: load the stub index, jump to the shared
    // logger. Ten bytes, no faulting instructions, entered via the bound GOT
    // slot with any signature.
    std::vector<std::uint32_t> stubTrampolines;
    for (std::size_t uid = 0; uid < uniqueNames.size(); ++uid) {
        stubTrampolines.push_back(code.GetRva());
        code.Emit({0xb9});
        code.U32(static_cast<std::uint32_t>(uid));
        code.Rip({0xe9}, stubLoggerRva);
    }
    for (const auto& [offset, uid] : stubLeaFixups)
        code.PatchBranch(offset, stubTrampolines.at(uid));
    // Lazy unresolved imports are file-patched to the same ret-0 trampolines
    // (instead of upstream's fail stubs), so both modes stay consistent.
    for (const auto& [importIndex, uid] : lazyStubFixups)
        result.LazyStubs.push_back({imports[importIndex].TargetRva, stubTrampolines.at(uid)});

    Io::WriteU32(data, functionTable - dataRva, result.Code.Rva);
    Io::WriteU32(data, functionTable - dataRva + 4, functionEnd);
    Io::WriteU32(data, functionTable - dataRva + 8, unwindRva);
    Io::WriteU32(data, functionTable - dataRva + 12, stubLoggerRva);
    Io::WriteU32(data, functionTable - dataRva + 16, stubLoggerEnd);
    Io::WriteU32(data, functionTable - dataRva + 20, stubUnwindRva);
    auto exceptionEntries = CheckedRva(2);
    if (dependencyBuilder.has_value()) {
        const auto dependency = dependencyBuilder->Build(code, nativeImports);
        for (const auto offset : dependencyCalls)
            code.PatchBranch(offset, dependency.EntryRva);
        if (dependency.Functions.size() >= 31)
            throw Domain::RelinkerException("Too many dependency diagnostic routines");
        for (std::size_t index = 0; index < dependency.Functions.size(); ++index) {
            for (std::size_t field = 0; field < 3; ++field)
                Io::WriteU32(data, functionTable - dataRva + (index + 2) * 12 + field * 4, dependency.Functions[index][field]);
        }
        exceptionEntries = CheckedRva(dependency.Functions.size() + 2);
    }
    result.ExceptionDirectory = {functionTable, CheckedRva(exceptionEntries * 12)};
    result.Code.Data = code.TakeBytes();
    return result;
}

}
