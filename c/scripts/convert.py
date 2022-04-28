"""Convert.py

Tool used to convert the unpacked resource fork data (unpacked with DeRez) into data easily
usable within the Reckless Drivin' game.`
"""

import binascii
from io import BufferedWriter
import struct
import sys
from typing import ByteString

# Stored in a struct as
# struct Data {
#   char type[8];
#   uint32_t id;
#   uint32_t length;
#   char *data;
# };

class Header:
    def __init__(self, _type, id, desc=None):
        self.type = _type.decode("UTF-8")
        self.id = int(id.decode("UTF-8"))
        self.desc = desc.decode("UTF-8") if desc else None
    def __repr__(self) -> str:
        return f"{self.type} {self.id} {self.desc}"

    def write(self, _file: BufferedWriter) -> int:
        _file.write(self.type.encode("UTF-8"))
        _file.write(struct.pack("I", 0))
        _file.write(struct.pack("I", self.id))

class Data:
    def __init__(self, header: Header, data: ByteString):
        self.header = header
        self.bytes = data
        self.len = len(self.bytes)
    def __repr__(self) -> str:
        return f"{self.header} {self.len}"

    def write(self, _file: BufferedWriter, bytes_written: int) -> int:
        self.header.write(_file)
        _file.write(struct.pack("I", self.len))
        _file.write(self.bytes)
        return 0

# Header format
# data 'TYPE' (ID, "LONG DESCRIPTION") {
#
REPLACE_CHARS = b"(),'\"{"
def parse_block_header(header: bytes) -> dict:
    # Replace whitespace
    for char in REPLACE_CHARS:
        header = header.replace(bytes([char]), b" ")
    header_parts = header.split()

    has_description = bool(len(header_parts) > 3)
    data_type = header_parts[1]
    data_id = header_parts[2]
    data_description = None
    if has_description:
        data_description = b" ".join(header_parts[3:])

    return Header(data_type, data_id, data_description)

def parse_data_line(line: bytes) -> bytes:
    start = line.index(b'"')
    end = line.index(b'"', start + 1)

    hex_str = line[start + 1:end].decode("UTF-8")
    hex_str = "".join(hex_str.split())

    return binascii.unhexlify(hex_str)

def parse_data_block(lines, index) -> list:
    if not lines[index].startswith(b"data"):
        print("MALFORMED DATA FILE")
        sys.exit(1)

    header = parse_block_header(lines[index])
    print(header)

    index += 1
    data = b""
    while index < len(lines) and not lines[index].startswith(b"data"):
        if not lines[index]:
            index += 1
            continue
        if lines[index] == b"};":
            index += 1
            continue
        if lines[index].find(b"$"):
            data += parse_data_line(lines[index])
        index += 1

    return index, Data(header, data)

def parse_data_file(infile: str) -> list:
    num_data = 0
    with open(infile, 'rb') as _file:
        lines = _file.read().splitlines()

    i = 0
    blocks = []
    while i < len(lines):
        i, data = parse_data_block(lines, i)

        blocks.append(data)

        num_data += 1

    print(f"\nDATA: {num_data}")
    return blocks

def write_block(_file: BufferedWriter, block: Data, bytes_written: int) -> int:
    bytes_written += block.write(_file, bytes_written)
    return bytes_written

def write_output_file(outfile: str, blocks: list):
    bytes_written = 0
    with open(outfile, 'wb') as _file:
        for block in blocks:
            bytes_written += write_block(_file, block, bytes_written)

if (len(sys.argv) != 3):
    print(f"USAGE: {sys.argv[0]} infile outfile")
    sys.exit(1)

infile = sys.argv[1]
outfile = sys.argv[2]

blocks = parse_data_file(infile)
write_output_file(outfile, blocks)
