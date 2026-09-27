"""Exercise static (symbol-less) relocation passthrough through the real pipeline."""

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from test_optional_plt import fixture


def with_static_dtmod64():
    data = bytearray(fixture())
    # .rela lives at 0x700 with a single RELATIVE entry; append a static
    # DTPMOD64 (type 16, sym 0) like the ones found in libc.prx / libcohtml.
    struct.pack_into("<QQq", data, 0x718, 0x500, 16, 0)
    # DT_RELASZ (tag 8) is the 6th tag at 0x400: grow 24 -> 48.
    struct.pack_into("<Q", data, 0x400 + 5 * 16 + 8, 48)
    # The Linux patcher needs spare program-header slots (1 kept PT_LOAD +
    # 4 synthetic). Declare 3 extra slots as dummy PT_DYNAMIC entries, which
    # the segment filter skips, so keptCount stays 1 and 1 + 4 <= 5 fits.
    struct.pack_into("<H", data, 0x38, 5)
    for index in range(3):
        struct.pack_into("<IIQQQQQQ", data, 64 + (2 + index) * 56,
                         2, 0, 0, 0, 0, 0, 0, 0)
    return data


def main():
    relinker = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="anyps5-staticreloc-") as directory:
        work = Path(directory)
        source = work / "static_tls.elf"
        source.write_bytes(with_static_dtmod64())

        linux_out = work / "static_tls.linux.elf"
        result = subprocess.run([str(relinker), str(source), str(linux_out)],
                                capture_output=True, text=True, timeout=20)
        assert result.returncode == 0, (result.stdout, result.stderr)
        image = linux_out.read_bytes()
        assert struct.pack("<QQq", 0x500, 16, 0) in image, "static DTPMOD64 was dropped"
        assert struct.pack("<QQq", 0x300, 8, 0x210) in image, "RELATIVE was dropped"

        windows_out = work / "static_tls.exe"
        result = subprocess.run([str(relinker), "--windows", str(source), str(windows_out)],
                                capture_output=True, text=True, timeout=20)
        assert (result.returncode == 2
                and "has no Windows PE equivalent" in result.stderr
                and not windows_out.exists()), result
    print("Static relocation integration tests passed")


if __name__ == "__main__":
    main()
