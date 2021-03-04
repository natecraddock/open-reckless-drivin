#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "quickdraw.h"

#define FLIP_SHORT(var) var = ntohs((var))
#define FLIP_LONG(var) var = ntohl((var))

static uint32_t rect_area(Rect *r) {
  return (r->right - r->left) * (r->bottom - r->top);
}
static uint16_t rect_width(Rect *r) {
  return (r->right - r->left);
}

static void flip_endianness_rect(Rect *rect) {
  FLIP_SHORT(rect->top);
  FLIP_SHORT(rect->left);
  FLIP_SHORT(rect->bottom);
  FLIP_SHORT(rect->right);
}

void QD_flip_endianness(QuickDrawHeader *header) {
  FLIP_SHORT(header->picture_size);
  flip_endianness_rect(&header->bounding_rect);
  FLIP_SHORT(header->version_op_op);
  FLIP_SHORT(header->version_op);
  FLIP_SHORT(header->header_op);
  FLIP_SHORT(header->version);
  FLIP_SHORT(header->reserved_1);
  FLIP_LONG(header->horizontal_dpi);
  FLIP_LONG(header->vertical_dpi);
  flip_endianness_rect(&header->optimal_rectangle);
  FLIP_LONG(header->reserved_2);

  /* Hardcoded comment */
  FLIP_SHORT(header->long_comment);
  FLIP_SHORT(header->comment_type);
  FLIP_SHORT(header->comment_length);

  FLIP_SHORT(header->def_hilite);
  flip_endianness_rect(&header->clip_rect);
}

static void flip_endianness_pixmap(PixMap *pix_map) {
  FLIP_LONG(pix_map->base_addr);
  FLIP_SHORT(pix_map->row_bytes);
  flip_endianness_rect(&pix_map->bounds);
  FLIP_SHORT(pix_map->pm_version);
  FLIP_SHORT(pix_map->pack_type);
  FLIP_SHORT(pix_map->pack_size);
  FLIP_LONG(pix_map->h_res);
  FLIP_LONG(pix_map->v_res);
  FLIP_SHORT(pix_map->pixel_type);
  FLIP_SHORT(pix_map->pixel_size);
  FLIP_SHORT(pix_map->component_count);
  FLIP_SHORT(pix_map->component_size);
  FLIP_LONG(pix_map->plane_byte);
  FLIP_LONG(pix_map->pm_table);
  FLIP_LONG(pix_map->pm_reserved);
}

void DBR_flip_endianness(DirectBitsRect *direct_bits) {
  flip_endianness_pixmap(&direct_bits->pix_map);
  flip_endianness_rect(&direct_bits->src_rect);
  flip_endianness_rect(&direct_bits->dst_rect);
  FLIP_SHORT(direct_bits->mode);
}

static void PBR_flip_endianness(PackBitsRect *pack_bits) {
  flip_endianness_pixmap(&pack_bits->pix_map);
  flip_endianness_rect(&pack_bits->src_rect);
  flip_endianness_rect(&pack_bits->dst_rect);
  FLIP_SHORT(pack_bits->mode);
}

static void CT_flip_endianness(ColorTable *color_table) {
  FLIP_LONG(color_table->seed);
  FLIP_SHORT(color_table->flags);
  FLIP_SHORT(color_table->size);
}

static uint8_t byte_read(char **bytes) {
  uint8_t val = *(uint8_t *)(*bytes);
  *bytes += sizeof(uint8_t);
  return val;
}

static uint16_t short_read(char **bytes) {
  uint16_t val = *(uint16_t *)(*bytes);
  *bytes += sizeof(uint16_t);
  return val;
}

static uint32_t int_read(char **bytes) {
  uint32_t val = *(uint32_t *)(*bytes);
  *bytes += sizeof(uint32_t);
  return val;
}

static Rect rect_read(char **bytes) {
  Rect rect;
  rect.top = short_read(bytes);
  rect.left = short_read(bytes);
  rect.bottom = short_read(bytes);
  rect.right = short_read(bytes);
  return rect;
}

static PixMap PM_read(char **bytes) {
  PixMap pix_map;
  pix_map.base_addr = int_read(bytes);
  pix_map.row_bytes = short_read(bytes);
  pix_map.bounds = rect_read(bytes);
  pix_map.pm_version = short_read(bytes);
  pix_map.pack_type = short_read(bytes);
  pix_map.pack_size = int_read(bytes);
  pix_map.h_res = int_read(bytes);
  pix_map.v_res = int_read(bytes);
  pix_map.pixel_type = short_read(bytes);
  pix_map.pixel_size = short_read(bytes);
  pix_map.component_count = short_read(bytes);
  pix_map.component_size = short_read(bytes);
  pix_map.plane_byte = int_read(bytes);
  pix_map.pm_table = int_read(bytes);
  pix_map.pm_reserved = int_read(bytes);
  return pix_map;
}

static void CT_read_array(char **bytes, ColorTable *color_table) {
  ColorSpec color_spec;
  for (uint16_t index = 0; index <= color_table->size; index++) {
    color_spec.value = short_read(bytes);
    FLIP_SHORT(color_spec.value);

    /* The two bytes read for the rbg components are identical so for our
     * purposes it is safe to just store them in a byte. */
    color_spec.rgb.g = short_read(bytes);
    color_spec.rgb.r = short_read(bytes);
    color_spec.rgb.b = short_read(bytes);

    color_table->color_spec_array[index] = color_spec;
  }
}

static ColorTable CT_read(char **bytes) {
  ColorTable color_table;
  color_table.seed = int_read(bytes);
  color_table.flags = short_read(bytes);
  /* Size stores one less than number of entries */
  color_table.size = short_read(bytes);
  CT_flip_endianness(&color_table);

  color_table.color_spec_array =
      malloc(sizeof *color_table.color_spec_array * (color_table.size + 1));
  CT_read_array(bytes, &color_table);

  return color_table;
}

static uint16_t read_scanline(char **bytes, PixMap *pix_map, RGBValue *pixels,
                              uint16_t line) {
  const bool packed_bytes = pix_map->pixel_size == 8;
  const bool indexed = pix_map->pixel_type == 0;

  uint16_t byte_count = 0;
  /* Mask out top two bits */
  uint16_t row_bytes = (pix_map->row_bytes & 0x3FFF);

  if (row_bytes > 250) {
    byte_count = short_read(bytes);
    FLIP_SHORT(byte_count);
  }
  else {
    /* I don't think this will apply to any images in reckless drivin' */
    byte_count = byte_read(bytes);
  }

  /* Allocate enough memory to expand the line */
  uint8_t *buf = malloc(row_bytes);
  uint16_t index = 0;

  /* Run length decode the PackBits */
  while (index < row_bytes) {
    uint8_t control = byte_read(bytes);
    uint8_t repeat = 1;
    if (0 <= control && control <= 127) {
      /* Interpret next control+1 bytes literally */
      repeat = control + 1;
      if (packed_bytes) {
        for (uint8_t i = 0; i < repeat; i++, index++) {
          buf[index] = byte_read(bytes);
        }
      }
      else {
        for (uint8_t i = 0; i < repeat; i++, index += 2) {
          *(uint16_t *)(buf + index) = short_read(bytes);
          FLIP_SHORT(*(uint16_t *)(buf + index));
        }
      }
    }
    else if (control == 128) {
      /* skip this byte */
    }
    else {
      /* Repeat the next byte 257-control times */
      repeat = 257 - control;

      if (packed_bytes) {
        uint8_t value = byte_read(bytes);
        for (uint8_t i = 0; i < repeat; i++, index++) {
          buf[index] = value;
        }
      }
      else {
        uint16_t value = short_read(bytes);
        FLIP_SHORT(value);
        for (uint8_t i = 0; i < repeat; i++, index += 2) {
          *(uint16_t *)(buf + index) = value;
        }
      }
    }
  }

  index = 0;
  uint16_t width = rect_width(&pix_map->bounds);
  if (indexed) {
    PackBitsRect *pack_bits = (PackBitsRect *)pix_map;
    ColorTable color_table = pack_bits->color_table;
    for (uint16_t pixel = 0; pixel < width; pixel++) {
      uint16_t index = buf[pixel];
      pixels[(line * width) + pixel] = color_table.color_spec_array[index].rgb;
    }
  }
  else {
    for (uint16_t pixel = 0; pixel < width; pixel++, index += 2) {
      RGBValue rgb;
      uint16_t color = *(uint16_t *)(buf + index);

      rgb.r = (color & 0x7C00) >> 10;
      rgb.g = (color & 0x03E0) >> 5;
      rgb.b = (color & 0x001F);

      pixels[(line * width) + pixel] = rgb;
    }
  }

  free(buf);
  return byte_count;
}

static void read_pixels(char **bytes, PixMap *pix_map, RGBValue *pixels) {
  uint16_t num_scan_lines = 0;

  if (pix_map->pack_type > 2 || pix_map->pack_type == 0) {
    Rect bounds = pix_map->bounds;
    num_scan_lines = bounds.bottom - bounds.top;

    uint32_t count = 0;
    for (uint16_t line = 0; line < num_scan_lines; line++) {
      count += read_scanline(bytes, pix_map, pixels, line);
    }
  }
}

DirectBitsRect DBR_read(char **bytes) {
  DirectBitsRect direct_bits;
  direct_bits.pix_map = PM_read(bytes);
  direct_bits.src_rect = rect_read(bytes);
  direct_bits.dst_rect = rect_read(bytes);
  direct_bits.mode = short_read(bytes);
  DBR_flip_endianness(&direct_bits);

  direct_bits.pixels = malloc(sizeof *direct_bits.pixels *
                              rect_area(&direct_bits.pix_map.bounds));
  read_pixels(bytes, &direct_bits.pix_map, direct_bits.pixels);

  return direct_bits;
}

PackBitsRect PBR_read(char **bytes) {
  PackBitsRect pack_bits;
  /* The pack bits doesn't have the first field of the PixMap for some reason...
    The simplest solution is to just move back 4 bytes then read "junk" for that
    first field.*/
  *bytes -= sizeof(uint32_t);
  pack_bits.pix_map = PM_read(bytes);
  pack_bits.color_table = CT_read(bytes);
  pack_bits.src_rect = rect_read(bytes);
  pack_bits.dst_rect = rect_read(bytes);
  pack_bits.mode = short_read(bytes);
  PBR_flip_endianness(&pack_bits);

  pack_bits.pixels =
      malloc(sizeof *pack_bits.pixels * rect_area(&pack_bits.pix_map.bounds));
  read_pixels(bytes, &pack_bits.pix_map, pack_bits.pixels);

  return pack_bits;
}
