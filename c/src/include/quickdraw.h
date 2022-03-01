// An implementation of a (tiny) subset of the Apple Quickdraw 2D Graphics
// library.

// Notes:
// Type Integer is 2 bytes
// Type LongInt is 4 bytes

// Type Rect
//  top:    Integer
//  left:   Integer
//  bottom: Integer
//  right:  Integer

// Type DirectBitsRect
//  PixMap: PixMap
//  srcRect: Rect
//  dstRect: Rect
//  mode:    Integer (transfer mode) ditherCopy?
//  PixData: Bytes (variable)

// Type PixMap:
//  baseAddr:   Ptr (should have value 0x000000FF)
//  rowBytes:   Integer (flags (2 high bits) and row width)
//  bounds:     Rect
//  pmVersion:  Integer (PixMap record version number)
//              normally zero, if 4 then baseAddr is a 32-bit clean
//  packType:   Integer (packing format)
//              0: no packing 1-4: packed
//  packSize:   LongInt (size of data in packed state)
//              0 when packType is zero, otherwise size of packed image
//  hRes:       Fixed - usually 0x00480000 for 72 ppi
//  vRes:       Fixed - usually 0x00480000 for 72 ppi
//  pixelType:  Integer (format of pixel image)
//  pixelSize:  Integer (physical bits per pixel)
//  cmpCount:   Integer (logical components per pixel)
//  cmpSize:    Integer (logical bits per component)
//  planeByte:  LongInt (offset to nextplane)
//  pmTable:    CTabHandle (handle to colortable record)
//  pmReserved: LongInt

// When packType is 3 then
//  each image contains of bounds.bottom - bounds.top packed (compressed)
//  scan lines.
//  Each scan line consists of a block of data of the form
//   byteCount: Integer
//   data:      Variable of length byteCount

#include <stdint.h>

#include "defines.h"

typedef struct Rect {
  uint16_t top;
  uint16_t left;
  uint16_t bottom;
  uint16_t right;
} Rect;

typedef struct QuickDrawHeader {
  uint16_t picture_size;
  Rect bounding_rect;
  uint16_t version_op_op;
  uint16_t version_op;
  uint16_t header_op;
  uint16_t version;
  uint16_t reserved_1;
  uint32_t horizontal_dpi;
  uint32_t vertical_dpi;
  Rect optimal_rectangle;
  uint32_t reserved_2;

  // Hardcode comment for now
  uint16_t long_comment;
  uint16_t comment_type;
  uint16_t comment_length;
  uint8_t comment[22];

  uint16_t def_hilite;
  uint16_t region_size;
  Rect clip_rect;
} QuickDrawHeader;

typedef struct PixMap {
  uint32_t base_addr;
  uint16_t row_bytes;
  Rect bounds;
  uint16_t pm_version;
  uint16_t pack_type;
  uint32_t pack_size;
  uint32_t h_res;
  uint32_t v_res;
  uint16_t pixel_type;
  uint16_t pixel_size;
  uint16_t component_count;
  uint16_t component_size;
  uint32_t plane_byte;
  uint32_t pm_table;
  uint32_t pm_reserved;
} PixMap;

typedef struct RGBValue {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGBValue;

typedef struct ColorSpec {
  uint16_t value;
  RGBValue rgb;
} ColorSpec;

typedef struct ColorTable {
  uint32_t seed;
  uint16_t flags;
  uint16_t size;
  ColorSpec *color_spec_array;
} ColorTable;

typedef struct DirectBitsRect {
  PixMap pix_map;
  Rect src_rect;
  Rect dst_rect;
  uint16_t mode;
  RGBValue *pixels;
} DirectBitsRect;

typedef struct PackBitsRect {
  PixMap pix_map;
  ColorTable color_table;
  Rect src_rect;
  Rect dst_rect;
  uint16_t mode;
  RGBValue *pixels;
} PackBitsRect;

void QD_flip_endianness(QuickDrawHeader *header);
void DBR_flip_endianness(DirectBitsRect *direct_bits);

DirectBitsRect DBR_read(char **bytes);
PackBitsRect PBR_read(char **bytes);
