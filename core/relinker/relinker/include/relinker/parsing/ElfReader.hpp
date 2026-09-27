#ifndef RELINKER_PARSING_ELFREADER_HPP
#define RELINKER_PARSING_ELFREADER_HPP

#include <relinker/domain/IElfReader.hpp>
#include <vector>

namespace Relinker {

class ElfReader : public IElfReader {
public:
    explicit ElfReader(std::vector<std::uint8_t> fileBuffer);

    // PS5 SELF support (plaintext dumps, e.g. eboot.bin with magic 4F153D1D).
    // MaybeUnwrapSelfImage returns the unwrapped ELF image when the input is a
    // SELF container, otherwise returns the input unchanged.
    static bool IsSelfImage(const std::vector<std::uint8_t>& bytes);
    static std::vector<std::uint8_t> UnwrapSelfImage(const std::vector<std::uint8_t>& bytes);
    static std::vector<std::uint8_t> MaybeUnwrapSelfImage(std::vector<std::uint8_t> bytes);

    ElfHeader ReadHeader() const override;
    std::vector<ProgramHeader> ReadProgramHeaders() const override;
    std::vector<SectionHeader> ReadSectionHeaders() const override;
    std::vector<DynamicTag> ReadDynamicTags(const ProgramHeader& dynamicHeader) const override;
    FileByteOffset TranslateVirtualAddress(VirtualAddress address) const override;
    std::vector<std::uint8_t> ReadSection(const SectionHeader& header) const override;
    std::vector<std::uint8_t> ReadSegment(const ProgramHeader& header) const override;
    std::vector<ProgramHeader> ReadCodeSegments() const override;
    std::uint64_t GetFileSize() const override;
    const std::vector<std::uint8_t>& GetRawBytes() const override;

private:
    std::vector<std::uint8_t> _fileBuffer;

    std::uint64_t _readU64At(FileByteOffset offset) const;
    std::uint32_t _readU32At(FileByteOffset offset) const;
    std::uint16_t _readU16At(FileByteOffset offset) const;
    std::uint8_t _readU8At(FileByteOffset offset) const;
    std::string _resolveShdrName(std::uint32_t nameOffset, const ElfHeader& header) const;
};

}

#endif
