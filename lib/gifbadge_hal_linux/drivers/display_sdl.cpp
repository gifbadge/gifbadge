#include <cstdlib>
#include "drivers/display_sdl.h"

#include <cstdio>

#include "log.h"

hal::display::oslinux::display_sdl *displaySdl = nullptr;

hal::display::oslinux::display_sdl::display_sdl() {
  sem_init(&mutex, 0, 0);
  size = {480, 480};
  buffer = static_cast<uint8_t *>(malloc(480*480*2));
  if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
    LOGI("hal::display::oslinux::display_sdl", "error initializing SDL: %s\n", SDL_GetError());
  }

  win = SDL_CreateWindow("GAME",480, 480, 0);

  renderer = SDL_CreateRenderer(win, nullptr);
  SDL_SetRenderDrawColor(renderer, 255, 255,
                         255, 255);
  SDL_RenderClear(renderer);
  pixels = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 480, 480);

}
hal::display::oslinux::display_sdl::~display_sdl() {
  SDL_DestroyTexture(pixels);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();
  if (buffer != nullptr) {
    free(buffer);
  }
  sem_destroy(&mutex);
}
bool hal::display::oslinux::display_sdl::onColorTransDone(flushCallback_t callback) {
  _callback = callback;
  return false;
}
void hal::display::oslinux::display_sdl::write(int x_start, int y_start, int x_end, int y_end, const void *color_data) {
  if (color_data != buffer) {
    memcpy(buffer, color_data, 480 * 480 * 2);
  }
  sem_post(&mutex);
}
void hal::display::oslinux::display_sdl::update() {
  if(sem_trywait(&mutex) != -1) {
    void *data;
    int pitch;
    SDL_LockTexture(pixels, nullptr, &data, &pitch);

    memcpy(data, buffer, 480 * 480 * 2);

    SDL_UnlockTexture(pixels);

    // copy to window
    SDL_RenderTexture(renderer, pixels, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    if (_callback) {
      _callback();
    }
  }
}

void hal::display::oslinux::display_sdl::clear() {

}

hal::display::oslinux::display_sdl *display_sdl_init() {
  displaySdl = new hal::display::oslinux::display_sdl();
  return displaySdl;
}
