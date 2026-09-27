#include <relinker/parsing/ElfReader.hpp>
#include <relinker/domain/Types.hpp>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace Relinker {

namespace {

constexpr std::size_t kSelfHeaderSize = 32;
constexpr std::size_t kSelfSegmentSize = 32;
constexpr std::size_t kEhdrSize = 64;
constexpr std::size_t kPhdrSize = 56;

std::uint16_t readU16LE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint16_t value;
    std::memcpy(&value, bytes.data() + offset, 2);
    return value;
}

std::uint32_t readU32LE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint32_t value;
    std::memcpy(&value, bytes.data() + offset, 4);
    return value;
}

std::uint64_t readU64LE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint64_t value;
    std::memcpy(&value, bytes.data() + offset, 8);
    return value;
}

bool hasElfMagicAt(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return bytes.size() >= offset + 4 &&
        bytes[offset] == 0x7f && bytes[offset + 1] == 'E' &&
        bytes[offset + 2] == 'L' && bytes[offset + 3] == 'F';
}

} // namespace

ElfReader::ElfReader(std::vector<std::uint8_t> fileBuffer)
    : _fileBuffer(MaybeUnwrapSelfImage(std::move(fileBuffer)))
{
}

const std::vector<std::uint8_t>& ElfReader::GetRawBytes() const {
    return _fileBuffer;
}

std::uint8_t ElfReader::_readU8At(FileByteOffset fileByteOffset) const {
    if (fileByteOffset >= _fileBuffer.size()) {
        throw RelinkerException("FileByteOffset out of bounds", fileByteOffset);
    }
    return _fileBuffer[fileByteOffset];
}

std::uint16_t ElfReader::_readU16At(FileByteOffset fileByteOffset) const {
    if (fileByteOffset + 2 > _fileBuffer.size()) {
        throw RelinkerException("FileByteOffset out of bounds", fileByteOffset);
    }
    std::uint16_t value;
    std::memcpy(&value, _fileBuffer.data() + fileByteOffset, 2);
    return value;
}

std::uint32_t ElfReader::_readU32At(FileByteOffset fileByteOffset) const {
    if (fileByteOffset + 4 > _fileBuffer.size()) {
        throw RelinkerException("FileByteOffset out of bounds", fileByteOffset);
    }
    std::uint32_t value;
    std::memcpy(&value, _fileBuffer.data() + fileByteOffset, 4);
    return value;
}

std::uint64_t ElfReader::_readU64At(FileByteOffset fileByteOffset) const {
    if (fileByteOffset + 8 > _fileBuffer.size()) {
        throw RelinkerException("FileByteOffset out of bounds", fileByteOffset);
    }
    std::uint64_t value;
    std::memcpy(&value, _fileBuffer.data() + fileByteOffset, 8);
    return value;
}

ElfHeader ElfReader::ReadHeader() const {
    if (_fileBuffer.size() < 20) {
        throw RelinkerException("File too small for ELF header");
    }

    if (_fileBuffer[0] != 0x7f || _fileBuffer[1] != 'E' ||
        _fileBuffer[2] != 'L' || _fileBuffer[3] != 'F') {
        throw RelinkerException("Invalid ELF magic number");
    }

    ElfHeader header{};
    header.Machine = _readU16At(0x12);
    header.Type = _readU16At(0x10);
    header.OsAbi = _readU8At(0x07);
    header.AbiVersion = _readU8At(0x08);
    header.EntryPoint = _readU64At(0x18);
    header.ProgramHeaderOffset = _readU64At(0x20);
    header.SectionHeaderOffset = _readU64At(0x28);
    header.ProgramHeaderEntrySize = _readU16At(0x36);
    header.ProgramHeaderCount = _readU16At(0x38);
    header.SectionHeaderEntrySize = _readU16At(0x3a);
    header.SectionHeaderCount = _readU16At(0x3c);
    header.SectionHeaderStringIndex = _readU16At(0x3e);

    return header;
}

std::vector<ProgramHeader> ElfReader::ReadProgramHeaders() const {
    const ElfHeader header = ReadHeader();

    std::vector<ProgramHeader> headers;
    FileByteOffset offset = header.ProgramHeaderOffset;

    for (std::uint16_t i = 0; i < header.ProgramHeaderCount; ++i) {
        ProgramHeader ph{};
        ph.Type = _readU32At(offset);
        ph.Flags = _readU32At(offset + 0x04);
        ph.Offset = _readU64At(offset + 0x08);
        ph.MappedAddress = _readU64At(offset + 0x10);
        ph.PhysicalAddress = _readU64At(offset + 0x18);
        ph.FileSize = _readU64At(offset + 0x20);
        ph.MemorySize = _readU64At(offset + 0x28);
        ph.Alignment = _readU64At(offset + 0x30);

        headers.push_back(ph);
        offset += header.ProgramHeaderEntrySize;
    }

    return headers;
}

std::vector<SectionHeader> ElfReader::ReadSectionHeaders() const {
    const ElfHeader header = ReadHeader();

    std::vector<SectionHeader> headers;
    FileByteOffset offset = header.SectionHeaderOffset;

    for (std::uint16_t i = 0; i < header.SectionHeaderCount; ++i) {
        SectionHeader sh;
        const std::uint32_t nameOffset = _readU32At(offset);
        sh.Type = _readU32At(offset + 0x04);
        sh.Flags = _readU64At(offset + 0x08);
        sh.MappedAddress = _readU64At(offset + 0x10);
        sh.Offset = _readU64At(offset + 0x18);
        sh.SectionSize = _readU64At(offset + 0x20);
        sh.Link = _readU32At(offset + 0x28);
        sh.Info = _readU32At(offset + 0x2c);
        sh.EntrySize = _readU64At(offset + 0x30);

        sh.Name = _resolveShdrName(nameOffset, header);

        headers.push_back(sh);
        offset += header.SectionHeaderEntrySize;
    }

    return headers;
}

std::string ElfReader::_resolveShdrName(std::uint32_t nameOffset, const ElfHeader& header) const {
    if (header.SectionHeaderStringIndex == 0) {
        return "";
    }

    FileByteOffset shstrOffset = header.SectionHeaderOffset +
                        (header.SectionHeaderStringIndex * header.SectionHeaderEntrySize);

    const FileByteOffset strTableOffset = _readU64At(shstrOffset + 0x18);

    std::string name;
    FileByteOffset currentPos = strTableOffset + nameOffset;

    while (currentPos < _fileBuffer.size() && _fileBuffer[currentPos] != '\0') {
        name += static_cast<char>(_fileBuffer[currentPos]);
        currentPos++;
    }

    return name;
}

std::vector<DynamicTag> ElfReader::ReadDynamicTags(const ProgramHeader& dynamicHeader) const {
    std::vector<DynamicTag> tags;
    FileByteOffset offset = dynamicHeader.Offset;
    const FileByteOffset end = dynamicHeader.Offset + dynamicHeader.FileSize;

    while (offset + 16 <= end && offset + 16 <= _fileBuffer.size()) {
        DynamicTag tag;
        tag.Tag = static_cast<std::int64_t>(_readU64At(offset));
        tag.Value = _readU64At(offset + 0x08);

        if (tag.Tag == 0) {
            break;
        }

        tags.push_back(tag);
        offset += 16;
    }

    return tags;
}

FileByteOffset ElfReader::TranslateVirtualAddress(VirtualAddress address) const {
    const ElfHeader header = ReadHeader();

    FileByteOffset offset = header.ProgramHeaderOffset;

    for (std::uint16_t i = 0; i < header.ProgramHeaderCount; ++i) {
        const std::uint32_t type = _readU32At(offset);
        const FileByteOffset segOffset = _readU64At(offset + 0x08);
        const VirtualAddress segVAddr = _readU64At(offset + 0x10);
        const ByteCount segFileSize = _readU64At(offset + 0x20);

        static constexpr std::uint32_t PT_LOAD = 1;

        if (type == PT_LOAD && address >= segVAddr && address < segVAddr + segFileSize) {
            return segOffset + (address - segVAddr);
        }

        offset += header.ProgramHeaderEntrySize;
    }

    throw RelinkerException("Virtual address not mapped by any PT_LOAD segment", address);
}

std::vector<std::uint8_t> ElfReader::ReadSection(const SectionHeader& header) const {
    if (header.Offset + header.SectionSize > _fileBuffer.size()) {
        throw RelinkerException("Section offset out of bounds", header.Offset);
    }

    return std::vector<std::uint8_t>(
        _fileBuffer.begin() + header.Offset,
        _fileBuffer.begin() + header.Offset + header.SectionSize);
}

std::vector<std::uint8_t> ElfReader::ReadSegment(const ProgramHeader& header) const {
    if (header.Offset + header.FileSize > _fileBuffer.size()) {
        throw RelinkerException("Segment offset out of bounds", header.Offset);
    }

    return std::vector<std::uint8_t>(
        _fileBuffer.begin() + header.Offset,
        _fileBuffer.begin() + header.Offset + header.FileSize);
}

std::vector<ProgramHeader> ElfReader::ReadCodeSegments() const {
    static constexpr std::uint32_t kPtLoad = 1;
    static constexpr std::uint32_t kPfX = 0x1;
    std::vector<ProgramHeader> result;
    for (const auto& ph : ReadProgramHeaders()) {
        if (ph.Type == kPtLoad && (ph.Flags & kPfX)) {
            result.push_back(ph);
        }
    }
    return result;
}

std::uint64_t ElfReader::GetFileSize() const {
    return _fileBuffer.size();
}

bool ElfReader::IsSelfImage(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < kSelfHeaderSize) {
        return false;
    }
    const bool magicA = bytes[0] == 0x4f && bytes[1] == 0x15 &&
        bytes[2] == 0x3d && bytes[3] == 0x1d;
    const bool magicB = bytes[0] == 0x54 && bytes[1] == 0x14 &&
        bytes[2] == 0xf5 && bytes[3] == 0xee;
    if (!magicA && !magicB) {
        return false;
    }
    std::uint16_t segmentsNum = 0;
    std::memcpy(&segmentsNum, bytes.data() + 24, 2);
    if (segmentsNum == 0 || segmentsNum > 128) {
        return false;
    }
    const std::size_t elfBase = kSelfHeaderSize + static_cast<std::size_t>(segmentsNum) * kSelfSegmentSize;
    if (elfBase + kEhdrSize > bytes.size()) {
        return false;
    }
    return hasElfMagicAt(bytes, elfBase);
}

std::vector<std::uint8_t> ElfReader::UnwrapSelfImage(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < kSelfHeaderSize) {
        throw RelinkerException("File too small for SELF header");
    }
    const bool magicA = bytes[0] == 0x4f && bytes[1] == 0x15 &&
        bytes[2] == 0x3d && bytes[3] == 0x1d;
    const bool magicB = bytes[0] == 0x54 && bytes[1] == 0x14 &&
        bytes[2] == 0xf5 && bytes[3] == 0xee;
    if (!magicA && !magicB) {
        throw RelinkerException("Not a SELF image");
    }

    const std::uint16_t segmentsNum = readU16LE(bytes, 24);
    const std::uint64_t selfFileSize = readU64LE(bytes, 16);
    if (segmentsNum == 0 || segmentsNum > 128) {
        throw RelinkerException("Invalid SELF segment count");
    }
    const std::size_t elfBase = kSelfHeaderSize + static_cast<std::size_t>(segmentsNum) * kSelfSegmentSize;
    if (elfBase + kEhdrSize > bytes.size()) {
        throw RelinkerException("SELF truncated before embedded ELF header");
    }
    if (!hasElfMagicAt(bytes, elfBase)) {
        throw RelinkerException("SELF does not contain an ELF header at the expected offset");
    }

    struct RawSelfSeg {
        std::uint64_t type;
        std::uint64_t offset;
        std::uint64_t compSize;
        std::uint64_t decompSize;
    };
    std::vector<RawSelfSeg> selfSegs;
    selfSegs.reserve(segmentsNum);
    for (std::uint16_t i = 0; i < segmentsNum; ++i) {
        const std::size_t off = kSelfHeaderSize + static_cast<std::size_t>(i) * kSelfSegmentSize;
        if (off + kSelfSegmentSize > bytes.size()) {
            throw RelinkerException("SELF truncated in segment table");
        }
        RawSelfSeg seg{};
        seg.type = readU64LE(bytes, off);
        seg.offset = readU64LE(bytes, off + 8);
        seg.compSize = readU64LE(bytes, off + 16);
        seg.decompSize = readU64LE(bytes, off + 24);
        if (seg.offset + seg.compSize > bytes.size() ||
            seg.offset + seg.decompSize > bytes.size() + (1u << 20)) {
            // Decompressed size may equal filesz; physical bytes must exist for compSize.
            if (seg.offset + seg.compSize > bytes.size()) {
                throw RelinkerException("SELF segment out of bounds", seg.offset);
            }
        }
        if (seg.compSize != seg.decompSize) {
            throw RelinkerException("Compressed SELF segment not supported (plaintext dumps only)", seg.offset);
        }
        selfSegs.push_back(seg);
    }

    std::cout << "SELF detected: magic ";
    for (int i = 0; i < 4; ++i) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", bytes[i]);
        std::cout << buf;
    }
    std::cout << " segments=" << segmentsNum << " elfBase=0x" << std::hex << elfBase << std::dec << "\n";
    for (std::uint16_t i = 0; i < segmentsNum; ++i) {
        const auto& seg = selfSegs[i];
        const std::uint64_t phdrId = (seg.type >> 20u) & 0xFFFu;
        std::cout << "  SELF seg[" << i << "]: type=0x" << std::hex << seg.type << std::dec
            << " (phdr_id=" << phdrId << (seg.type & 0x800u ? ", mapped" : ", meta") << ")"
            << " offset=0x" << std::hex << seg.offset
            << " comp=0x" << seg.compSize << " decomp=0x" << seg.decompSize << std::dec << "\n";
    }

    const std::uint64_t ePhoff = readU64LE(bytes, elfBase + 0x20);
    const std::uint16_t ePhentsize = readU16LE(bytes, elfBase + 0x36);
    const std::uint16_t ePhnum = readU16LE(bytes, elfBase + 0x38);
    if (ePhentsize != kPhdrSize) {
        throw RelinkerException("Unsupported SELF program header entry size");
    }
    if (ePhnum == 0 || ePhnum > 100) {
        throw RelinkerException("Invalid SELF program header count");
    }
    if (elfBase + ePhoff + static_cast<std::uint64_t>(ePhnum) * kPhdrSize > bytes.size()) {
        throw RelinkerException("SELF program header table out of bounds");
    }

    struct RawPhdr {
        std::uint32_t type;
        std::uint32_t flags;
        std::uint64_t offset;
        std::uint64_t vaddr;
        std::uint64_t filesz;
    };
    std::vector<RawPhdr> phdrs;
    phdrs.reserve(ePhnum);
    for (std::uint16_t i = 0; i < ePhnum; ++i) {
        const std::size_t off = elfBase + static_cast<std::size_t>(ePhoff) + static_cast<std::size_t>(i) * kPhdrSize;
        RawPhdr ph{};
        ph.type = readU32LE(bytes, off);
        ph.flags = readU32LE(bytes, off + 4);
        ph.offset = readU64LE(bytes, off + 8);
        ph.vaddr = readU64LE(bytes, off + 16);
        ph.filesz = readU64LE(bytes, off + 32);
        phdrs.push_back(ph);
    }

    struct Mapping {
        std::uint64_t logicalBase;
        std::uint64_t physBase;
        std::uint64_t size;
    };
    std::vector<Mapping> mappings;
    for (const auto& seg : selfSegs) {
        if ((seg.type & 0x800u) == 0) {
            continue;
        }
        const std::uint64_t pid = (seg.type >> 20u) & 0xFFFu;
        if (pid >= phdrs.size()) {
            throw RelinkerException("SELF segment references invalid program header", pid);
        }
        mappings.push_back({phdrs[pid].offset, seg.offset, seg.decompSize});
    }

    std::uint64_t unwrappedSize = kEhdrSize;
    if (ePhoff + static_cast<std::uint64_t>(ePhnum) * kPhdrSize > unwrappedSize) {
        unwrappedSize = ePhoff + static_cast<std::uint64_t>(ePhnum) * kPhdrSize;
    }
    for (const auto& ph : phdrs) {
        if (ph.filesz == 0) {
            continue;
        }
        if (ph.offset > (1ULL << 31) || ph.filesz > (1ULL << 31) ||
            ph.offset + ph.filesz < ph.offset) {
            throw RelinkerException("SELF program header has unreasonable range", ph.offset);
        }
        if (ph.offset + ph.filesz > unwrappedSize) {
            unwrappedSize = ph.offset + ph.filesz;
        }
    }
    if (unwrappedSize > (1ULL << 31)) {
        throw RelinkerException("Unwrapped SELF image too large");
    }

    std::vector<std::uint8_t> out(static_cast<std::size_t>(unwrappedSize), 0);
    std::memcpy(out.data(), bytes.data() + elfBase, kEhdrSize);
    // SELF containers carry no usable section table (Kyty skips it); clear the
    // fields so downstream code treats the image as section-less.
    std::memset(out.data() + 0x28, 0, 8); // e_shoff
    std::memset(out.data() + 0x3c, 0, 2); // e_shnum
    std::memset(out.data() + 0x3e, 0, 2); // e_shstrndx
    std::memcpy(out.data() + ePhoff, bytes.data() + elfBase + ePhoff,
        static_cast<std::size_t>(ePhnum) * kPhdrSize);

    const std::uint64_t headerEnd = ePhoff + static_cast<std::uint64_t>(ePhnum) * kPhdrSize;
    const auto isEssential = [](std::uint32_t type) {
        switch (type) {
            case 1: // PT_LOAD
            case 2: // PT_DYNAMIC
            case 3: // PT_INTERP
            case 6: // PT_PHDR
            case 7: // PT_TLS
            case 0x6474e550: // PT_GNU_EH_FRAME
            case 0x6474e552: // PT_GNU_RELRO
            case 0x61000000: // PT_OS_DYNLIBDATA
            case 0x61000001: // PT_OS_PROCPARAM
            case 0x61000010: // PT_OS_RELRO
                return true;
            default:
                return false;
        }
    };

    for (const auto& ph : phdrs) {
        if (ph.filesz == 0) {
            continue;
        }
        // The ELF/program headers themselves were already copied above.
        if (ph.offset + ph.filesz <= headerEnd) {
            continue;
        }
        std::uint64_t remaining = ph.filesz;
        std::uint64_t curLog = ph.offset;
        std::uint64_t curOut = ph.offset;
        bool tailUsed = false;
        while (remaining > 0) {
            bool progressed = false;
            for (const auto& map : mappings) {
                if (curLog >= map.logicalBase && curLog < map.logicalBase + map.size) {
                    const std::uint64_t chunk =
                        std::min(remaining, map.logicalBase + map.size - curLog);
                    const std::uint64_t phys = map.physBase + (curLog - map.logicalBase);
                    if (phys + chunk > bytes.size()) {
                        throw RelinkerException("SELF payload out of bounds", phys);
                    }
                    std::memcpy(out.data() + curOut, bytes.data() + phys,
                        static_cast<std::size_t>(chunk));
                    curLog += chunk;
                    curOut += chunk;
                    remaining -= chunk;
                    progressed = true;
                    break;
                }
            }
            if (progressed) {
                continue;
            }
            // Kyty fallback: trailing blob stored at selfFileSize when its size
            // exactly matches the requested segment (e.g. Legends phdr12).
            if (!tailUsed && curLog == ph.offset && remaining == ph.filesz &&
                selfFileSize <= bytes.size() && bytes.size() - selfFileSize == ph.filesz) {
                std::memcpy(out.data() + curOut, bytes.data() + selfFileSize,
                    static_cast<std::size_t>(ph.filesz));
                remaining = 0;
                tailUsed = true;
                std::cout << "  SELF phdr type=0x" << std::hex << ph.type
                    << std::dec << " off=0x" << std::hex << ph.offset
                    << std::dec << " filled from SELF tail\n";
                break;
            }
            if (!isEssential(ph.type)) {
                std::cout << "  SELF phdr type=0x" << std::hex << ph.type
                    << std::dec << " off=0x" << std::hex << ph.offset
                    << " size=0x" << ph.filesz << std::dec
                    << " has no SELF coverage; zero-filled\n";
                break;
            }
            throw RelinkerException("No SELF segment covers program header payload", curLog);
        }
    }

    std::cout << "SELF unwrapped: " << bytes.size() << " -> " << out.size() << " bytes\n";
    return out;
}

std::vector<std::uint8_t> ElfReader::MaybeUnwrapSelfImage(std::vector<std::uint8_t> bytes) {
    if (!IsSelfImage(bytes)) {
        return bytes;
    }
    return UnwrapSelfImage(bytes);
}

}
