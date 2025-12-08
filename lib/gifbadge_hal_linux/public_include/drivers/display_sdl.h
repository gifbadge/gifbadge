#pragma once

#include "hal/display.h"
#include <SDL2/SDL.h>
#include <semaphore.h>
#include "FreeRTOS.h"
#include "task.h"

namespace hal::display::oslinux {

class display_sdl : public hal::display::Display {
 public:
  void clear() override;

  display_sdl();
  ~display_sdl() override = default;

  bool onColorTransDone(flushCallback_t) override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void update();
 private:
  SDL_Window* win;
  SDL_Renderer * renderer;
  SDL_Surface* surface;
  SDL_Texture* tex;
  SDL_Surface *window_surface;
  SDL_Texture* pixels;
  SDL_GLContext context;

  sem_t mutex;
  flushCallback_t _callback = nullptr;
  TaskHandle_t _refreshTask;
};

void display_sdl_init();

extern display_sdl *displaySdl;

}