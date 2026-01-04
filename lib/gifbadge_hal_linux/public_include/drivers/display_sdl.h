/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "hal/display.h"
#include <SDL3/SDL.h>
#include <semaphore.h>
#include "FreeRTOS.h"
#include "task.h"

namespace hal::display::oslinux {

class display_sdl : public hal::display::Display {
 public:
  void clear() override;

  display_sdl();
  ~display_sdl() override;

  bool onColorTransDone(flushCallback_t) override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void update();
 private:
  SDL_Window* win = nullptr;
  SDL_Renderer * renderer = nullptr;
  SDL_Surface* surface = nullptr;
  SDL_Texture* tex = nullptr;
  SDL_Surface *window_surface = nullptr;
  SDL_Texture* pixels = nullptr;
  SDL_GLContext context = nullptr;

  sem_t mutex;
  flushCallback_t _callback = nullptr;
};




}
hal::display::oslinux::display_sdl *display_sdl_init();

extern hal::display::oslinux::display_sdl *displaySdl;