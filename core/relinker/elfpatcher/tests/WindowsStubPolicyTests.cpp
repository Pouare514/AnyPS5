#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <elfpatcher/windows/WindowsEntryStubBuilder.hpp>
#include <elfpatcher/windows/WindowsPeWriter.hpp>
#include <elfpatcher/windows/WindowsImportBuilder.hpp>
#include <io/BufferUtils.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace Fs = std::filesystem;
using namespace Elfpatcher::Windows;

constexpr const char* MissingNid = "RelinkerStubPolicyMissingNid";

void writeFile(const Fs::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Cannot write stub policy fixture");
}

std::size_t countOccurrences(const std::string& haystack, const std::string& needle) {
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = haystack.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

// Builds a runnable PE whose startup bootstrap is produced by
// WindowsEntryStubBuilder. `imports` drives the GOT slots; the fake ELF entry
// calls the first GOT slot twice (checking the stub returns 0 the first time
// via int3 on mismatch) and then returns 0.
void createRunner(const Fs::path& path, const std::vector<PeImport>& imports, const std::size_t stubSlot, const bool lazyBinding = false) {
    auto nativeImports = WindowsImportBuilder().Build(LoadRva);
    const auto gotRva = AlignRva(nativeImports.Section.Data.size() + LoadRva);
    PeSection got{".gotslot", gotRva, SectionRead | SectionWrite | 0x40u, std::vector<std::uint8_t>(imports.size() * 8)};
    const auto dataRva = AlignRva(gotRva + got.Data.size());

    char systemDirectory[MAX_PATH]{};
    const auto systemSize = GetSystemDirectoryA(systemDirectory, sizeof(systemDirectory));
    if (systemSize == 0 || systemSize >= sizeof(systemDirectory))
        throw std::runtime_error("Cannot locate the system directory");
    const std::string runPath = std::string(systemDirectory) + "\\";

    std::vector<PeImport> slots;
    for (std::size_t index = 0; index < imports.size(); ++index)
        slots.push_back({imports[index].Name, CheckedRva(gotRva + index * 8), imports[index].Addend});
    auto entry = WindowsEntryStubBuilder().Build(dataRva, 0, nativeImports, {"KERNEL32.dll"}, slots, runPath, lazyBinding, false);
    if (entry.ExceptionDirectory.Size != 24)
        throw std::runtime_error("Stub policy bootstrap must register exactly two unwind entries");
    if (!entry.LazyStubs.empty())
        throw std::runtime_error("Stub policy must not emit lazy fail stubs (ret-0 in both modes)");

    // Fake ELF entry: when stubSlot is valid, call that GOT slot twice, int3
    // if the first call did not return 0 (proves the stub returns 0 with any
    // signature). Otherwise just return 0.
    const auto fakeRva = AlignRva(entry.Code.Rva + entry.Code.Data.size());
    std::vector<std::uint8_t> fake;
    const auto emitCallSlot = [&](const std::size_t slot) {
        const auto callRva = CheckedRva(fakeRva + fake.size());
        fake.insert(fake.end(), {0xff, 0x15});
        Io::AppendU32(fake, slots.at(slot).TargetRva - (callRva + 6));
    };
    if (stubSlot < slots.size()) {
        // Keep the ABI stack alignment: the startup bootstrap calls this
        // entry with rsp%16==8, so reserve 8 bytes before making calls.
        fake.insert(fake.end(), {0x48, 0x83, 0xec, 0x08});
        emitCallSlot(stubSlot);
        fake.insert(fake.end(), {0x48, 0x85, 0xc0, 0x75, 0x0d});
        emitCallSlot(stubSlot);
        fake.insert(fake.end(), {0x48, 0x83, 0xc4, 0x08});
    }
    fake.insert(fake.end(), {0x31, 0xc0, 0xc3, 0xcc});
    PeSection fakeEntry{".fakeelf", fakeRva, SectionRead | SectionExecute | 0x20u, fake};

    // Rebuild with the real ELF entry point now that its RVA is known. The
    // rebuild is byte-identical to the sizing build except for the entry
    // call target, since emitted code sizes do not depend on RVA values.
    auto retry = WindowsEntryStubBuilder().Build(dataRva, fakeRva, nativeImports, {"KERNEL32.dll"}, slots, runPath, lazyBinding, false);
    if (retry.ExceptionDirectory.Size != 24)
        throw std::runtime_error("Stub policy bootstrap must register exactly two unwind entries");
    if (!retry.LazyStubs.empty())
        throw std::runtime_error("Stub policy must not emit lazy fail stubs (ret-0 in both modes)");

    std::array<PeDirectory, 16> directories{};
    directories[1] = nativeImports.Directory;
    directories[3] = retry.ExceptionDirectory;
    directories[12] = nativeImports.AddressTable;
    std::vector<PeSection> sections;
    sections.push_back(std::move(nativeImports.Section));
    sections.push_back(std::move(got));
    sections.push_back(std::move(retry.Data));
    sections.push_back(std::move(retry.Code));
    sections.push_back(std::move(fakeEntry));
    writeFile(path, WindowsPeWriter().Write(sections, sections[3].Rva, directories));
}

struct RunResult {
    DWORD status;
    std::string output;
};

RunResult runExe(const Fs::path& runner) {
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE input = nullptr;
    HANDLE output = nullptr;
    if (!CreatePipe(&input, &output, &security, 0) || !SetHandleInformation(input, HANDLE_FLAG_INHERIT, 0))
        throw std::runtime_error("Cannot create stub policy test pipe");
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output;
    startup.hStdError = output;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    const auto filename = runner.string();
    if (!CreateProcessA(filename.c_str(), nullptr, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, runner.parent_path().string().c_str(), &startup, &process))
        throw std::runtime_error("Cannot start stub policy test: " + std::to_string(GetLastError()));
    CloseHandle(output);
    if (WaitForSingleObject(process.hProcess, 20000) != WAIT_OBJECT_0) {
        TerminateProcess(process.hProcess, 1);
        throw std::runtime_error("Stub policy test timed out");
    }
    DWORD status = 0;
    if (!GetExitCodeProcess(process.hProcess, &status))
        throw std::runtime_error("Cannot read stub policy exit code");
    std::string message;
    char buffer[4096];
    DWORD size = 0;
    while (ReadFile(input, buffer, sizeof(buffer), &size, nullptr) && size != 0)
        message.append(buffer, size);
    CloseHandle(input);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return {status, message};
}

void checkStubbedBoot(const Fs::path& directory) {
    const auto runner = directory / "stub-runner.exe";
    const std::vector<PeImport> imports = {
        {"LoadLibraryExA", 0, 0},
        {MissingNid, 0, 0},
        {MissingNid, 0, 0},
    };
    createRunner(runner, imports, 1);
    const auto result = runExe(runner);
    if (result.status != 0)
        throw std::runtime_error("Stubbed boot died with status " + std::to_string(result.status) + "\n" + result.output);
    if (result.output.find("FAIL") != std::string::npos)
        throw std::runtime_error("Stubbed boot must not log FAIL\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: unresolved ELF import ") + MissingNid + "\n") != 1)
        throw std::runtime_error("Missing NID must be logged once at bind time\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: called unresolved ELF import ") + MissingNid) != 1)
        throw std::runtime_error("Stub firing must be logged exactly once for two calls\n" + result.output);
    if (result.output.find("Transferring control to ELF entry point") == std::string::npos)
        throw std::runtime_error("Stubbed boot must reach the ELF entry point\n" + result.output);
}

void checkCleanBoot(const Fs::path& directory) {
    const auto runner = directory / "clean-runner.exe";
    const std::vector<PeImport> imports = {
        {"LoadLibraryExA", 0, 0},
    };
    createRunner(runner, imports, SIZE_MAX);
    const auto result = runExe(runner);
    if (result.status != 0)
        throw std::runtime_error("Clean boot died with status " + std::to_string(result.status) + "\n" + result.output);
    if (result.output.find("STUB") != std::string::npos)
        throw std::runtime_error("Clean boot must not log any STUB lines\n" + result.output);
    if (result.output.find("Transferring control to ELF entry point") == std::string::npos)
        throw std::runtime_error("Clean boot must reach the ELF entry point\n" + result.output);
}

void checkLazyStubbedBoot(const Fs::path& directory) {
    // Windows lazy binding is aligned to ret-0 stubs: same expectations as
    // eager, boot proceeds with identical STUB lines instead of faulting.
    const auto runner = directory / "stub-lazy-runner.exe";
    const std::vector<PeImport> imports = {
        {"LoadLibraryExA", 0, 0},
        {MissingNid, 0, 0},
        {MissingNid, 0, 0},
    };
    createRunner(runner, imports, 1, true);
    const auto result = runExe(runner);
    if (result.status != 0)
        throw std::runtime_error("Lazy stubbed boot died with status " + std::to_string(result.status) + "\n" + result.output);
    if (result.output.find("FAIL") != std::string::npos)
        throw std::runtime_error("Lazy stubbed boot must not log FAIL\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: unresolved ELF import ") + MissingNid + "\n") != 1)
        throw std::runtime_error("Lazy missing NID must be logged once at bind time\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: called unresolved ELF import ") + MissingNid) != 1)
        throw std::runtime_error("Lazy stub firing must be logged exactly once for two calls\n" + result.output);
    if (result.output.find("Transferring control to ELF entry point") == std::string::npos)
        throw std::runtime_error("Lazy stubbed boot must reach the ELF entry point\n" + result.output);
}

void checkAddendStubbedBoot(const Fs::path& directory) {
    // R_X86_64_64 nonzero-addend corner: GOT = stub address (addend skipped
    // for stubs to preserve callability). A called addend-8 stub must still
    // return 0 and boot, logging once like the addend-0 case.
    const auto runner = directory / "stub-addend-runner.exe";
    const std::vector<PeImport> imports = {
        {"LoadLibraryExA", 0, 0},
        {MissingNid, 0, 8},
    };
    createRunner(runner, imports, 1);
    const auto result = runExe(runner);
    if (result.status != 0)
        throw std::runtime_error("Addend stubbed boot died with status " + std::to_string(result.status) + "\n" + result.output);
    if (result.output.find("FAIL") != std::string::npos)
        throw std::runtime_error("Addend stubbed boot must not log FAIL\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: unresolved ELF import ") + MissingNid + "\n") != 1)
        throw std::runtime_error("Addend missing NID must be logged once at bind time\n" + result.output);
    if (countOccurrences(result.output, std::string("STUB: called unresolved ELF import ") + MissingNid) != 1)
        throw std::runtime_error("Addend stub firing must be logged exactly once for two calls\n" + result.output);
    if (result.output.find("Transferring control to ELF entry point") == std::string::npos)
        throw std::runtime_error("Addend stubbed boot must reach the ELF entry point\n" + result.output);
}

}

int main() {
    try {
        std::vector<char> filename(32768);
        const auto size = GetModuleFileNameA(nullptr, filename.data(), static_cast<DWORD>(filename.size()));
        if (size == 0 || size >= filename.size())
            throw std::runtime_error("Cannot locate stub policy tests");
        const auto directory = Fs::path(filename.data()).parent_path() / "windows-stub-policy-fixtures";
        Fs::create_directories(directory);
        checkStubbedBoot(directory);
        checkCleanBoot(directory);
        checkLazyStubbedBoot(directory);
        checkAddendStubbedBoot(directory);
        std::cout << "Windows stub policy machine-code tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
