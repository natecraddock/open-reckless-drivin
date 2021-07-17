#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>

#include "SDL2/SDL.h"

#include "initexit.h"

// #include "gameframe.h"
// #include "gameinitexit.h"
#include "interface.h"
#include "quickdraw.h"
#include "resource.h"
#include "lzrw.h"

typedef struct Display {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t *pixels;
} Display;

Display init_sdl() {
  SDL_Init(SDL_INIT_VIDEO);

  Display d;
  d.window = SDL_CreateWindow("Reckless Drivin'", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
  if (!d.window) {
    fprintf(stderr, "Unable to create window\n");
    exit(1);
  }

  d.renderer = SDL_CreateRenderer(d.window, -1, SDL_RENDERER_ACCELERATED);
  if (!d.renderer) {
    fprintf(stderr, "Unable to create renderer\n");
    exit(1);
  }

  d.texture = SDL_CreateTexture(d.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 640, 480);
  if (!d.texture) {
    fprintf(stderr, "Unable to create texture\n");
    exit(1);
  }

  d.pixels = calloc(640 * 480, sizeof (uint32_t));
  if (!d.pixels) {
    fprintf(stderr, "Unable to allocate pixel buffer\n");
    exit(1);
  }

  return d;
}


void cleanup_sdl(Display display) {
  SDL_DestroyRenderer(display.renderer);
  SDL_DestroyWindow(display.window);
  SDL_Quit();
}

void load_picture(Display display, Handle resource) {
  char *bytes = *resource;

  /* QuickDrawHeader */
  QuickDrawHeader *header = (QuickDrawHeader *)bytes;
  QD_flip_endianness(header);
  bytes += sizeof(QuickDrawHeader);

  /* Now read the DirectBitsRect */
  uint16_t op = *(uint16_t *)bytes;
  FLIP_SHORT(op);
  bytes += sizeof(uint16_t);

  /* DirectBitsRect */
  if (op == 0x009A) {
    DirectBitsRect direct_bits = DBR_read(&bytes);
    /* Copy colors to display buffer */
    uint32_t width = direct_bits.dst_rect.right;
    uint32_t height = direct_bits.dst_rect.bottom;
    for (uint32_t pix = 0; pix < width * height; pix++) {
      RGBValue rgb = direct_bits.pixels[pix];
      display.pixels[pix] = ((rgb.r * 8) << 16) | ((rgb.g * 8) << 8) | rgb.b * 8;
    }
  }
  /* PackBitsRect */
  else if (op == 0x0098) {
    PackBitsRect pack_bits = PBR_read(&bytes);
    uint32_t width = pack_bits.dst_rect.right;
    uint32_t height = pack_bits.dst_rect.bottom;
    for (uint32_t pix = 0; pix < width * height; pix++) {
      RGBValue rgb = pack_bits.pixels[pix];
      display.pixels[pix] = ((rgb.r * 8) << 16) | ((rgb.g * 8) << 8) | rgb.b * 8;
    }
  }

  ReleaseResource(resource);
}

void draw_picture(Display display, int pic_id){
  Handle pic = GetResource("PPic", pic_id);
  LZRWDecodeHandle(&pic);
  load_picture(display, pic);
}

int main(void) {
  printf("Open Reckless Drivin' 0.0.0\n");

  Display display = init_sdl();

  Init(display);
  draw_picture(display, 1003);
  SDL_Event event;
  while (!gExit) {
    SDL_UpdateTexture(display.texture, NULL, display.pixels, 640 * sizeof (uint32_t));
    /* if (gGameOn) { */
    /*   GameFrame(); */
    /* } */
    /* else { */
    /*   Eventloop(); */
    /* } */
    SDL_WaitEvent(&event);

    switch (event.type) {
    case SDL_QUIT:
      gExit = true;
      break;
    }

    SDL_RenderClear(display.renderer);
    SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);
    SDL_RenderPresent(display.renderer);
  }
  Exit();

  cleanup_sdl(display);

  return 0;
}
