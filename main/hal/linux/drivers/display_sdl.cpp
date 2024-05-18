#include <cstdlib>
#include "hal/linux/drivers/display_sdl.h"
#include "log.h"

display_sdl *displaySdl;

display_sdl::display_sdl() {
  sem_init(&mutex, 0, 1);
  size = {480, 480};
  buffer = static_cast<uint8_t *>(malloc(480*480*2));

  // returns zero on success else non-zero
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0) {
    printf("error initializing SDL: %s\n", SDL_GetError());
  }

  win = SDL_CreateWindow("GAME",
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     480, 480, 0);

  renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  SDL_SetRenderDrawColor(renderer, 255, 255,
                         255, 255);
  SDL_RenderClear(renderer);
  pixels = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 480, 480);

//  window_surface = SDL_GetWindowSurface(win);
//  surface = SDL_CreateRGBSurfaceWithFormat(0, 480, 480, 16, SDL_PIXELFORMAT_BGR565);


}
bool display_sdl::onColorTransDone(flushCallback_t callback) {
  _callback = callback;
  return false;
}
void display_sdl::write(int x_start, int y_start, int x_end, int y_end, const void *color_data) {
  sem_post(&mutex);
//  void* data;
//  int pitch;
////  SDL_GL_MakeCurrent(win, context);
//  SDL_LockTexture(pixels, NULL, &data, &pitch);
//
////  memcpy(data, color_data, 480*480*2);
////  SDL_memset(data, 0, pitch * 480);
//  SDL_UnlockTexture(pixels);
//
//  // copy to window
//  SDL_RenderCopy(renderer, pixels, NULL, NULL);
//  SDL_RenderPresent(renderer);
//  SDL_GL_MakeCurrent(win, nullptr);

//  SDL_LockSurface(surface);
//  memset(surface->pixels,255, 480*480*2);
////  memcpy(surface->pixels, color_data, 480*480*2);
//  SDL_UnlockSurface(surface);
//  SDL_UpperBlit(surface, nullptr, window_surface, nullptr);
//  SDL_UpdateWindowSurface(win);
}
void display_sdl::update() {
  sem_wait(&mutex);
  void* data;
  int pitch;
//  SDL_GL_MakeCurrent(win, context);
  SDL_LockTexture(pixels, NULL, &data, &pitch);

  memcpy(data, buffer, 480*480*2);
//  SDL_memset(data, 0, pitch * 480);
  SDL_UnlockTexture(pixels);

  // copy to window
  SDL_RenderCopy(renderer, pixels, NULL, NULL);
  SDL_RenderPresent(renderer);
  if(_callback){
    _callback();
  }
}
