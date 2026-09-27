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
    // Retained for API compatibility; always empty since the stub policy
    // binds unresolved imports to ret-0 trampolines in both eager and lazy
    // modes (Windows lazy has no effect, Linux uses DF_BIND_NOW).
    std::vector<WindowsLazyStub> LazyStubs;
};

class WindowsEntryStubBuilder {
public:
    WindowsEntryStub Build(std::uint32_t dataRva, std::uint32_t entryRva, const WindowsImports& nativeImports, const std::vector<std::string>& libraries, const std::vector<PeImport>& imports, const std::string& runPath, bool lazyBinding, bool dependencyDiagnostics) const;
};

}

#endif
