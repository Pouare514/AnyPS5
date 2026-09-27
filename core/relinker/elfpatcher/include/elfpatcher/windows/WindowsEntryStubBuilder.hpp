#ifndef ELFPATCHER_WINDOWS_ENTRYSTUBBUILDER_HPP
#define ELFPATCHER_WINDOWS_ENTRYSTUBBUILDER_HPP

#include <elfpatcher/windows/WindowsImportBuilder.hpp>

namespace Elfpatcher::Windows {

struct WindowsLazyStub {
    std::uint32_t TargetRva;
    std::uint32_t StubRva;
};

struct WindowsEntryStub {
    PeSection Data;
    PeSection Code;
    PeDirectory ExceptionDirectory;
    // Lazy-mode unresolved imports, bound by the file patcher to the shared
    // ret-0 trampolines (same as eager mode). Empty in eager mode.
    std::vector<WindowsLazyStub> LazyStubs;
};

class WindowsEntryStubBuilder {
public:
    WindowsEntryStub Build(std::uint32_t dataRva, std::uint32_t entryRva, const WindowsImports& nativeImports, const std::vector<std::string>& libraries, const std::vector<PeImport>& imports, const std::string& runPath, bool lazyBinding, bool dependencyDiagnostics, const std::vector<Domain::GuestRuntime>& guestModules = {}) const;
};

}

#endif
