#pragma once

#include <cstdio>
#include <cstdint>
#include <tuple>
#include <memory>
#include <map>
#include <array>
#include <span>

namespace image {
enum class frameStatus {
  OK,
  END,
  ERROR
};
typedef std::pair<frameStatus, uint32_t> frameReturn;
typedef std::pair<int16_t, int16_t> screenResolution;
class Image {
  protected:
  screenResolution resolution;

 public:
  explicit Image(screenResolution res) :resolution(res) {}

  virtual ~Image() = default;

  /**
   *
   * @param outBuf the buffer to write too
   * @param x offset in x direction
   * @param y offset in y direction
   * @param width width of the display
   * @return
   */
  virtual frameReturn GetFrame(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) { return {frameStatus::OK, 0}; };

  /**
   * Get the size of the image. Can only be called after image is opened with Open()
   * @return x, y pair representing the width and height of the image
   */
  virtual std::pair<int16_t, int16_t> Size() { return {0, 0}; };

  /**
   * Open an image from a path.
   * if buffer is provided, it will be used as working memory for image decoding
   * @param path null terminated string containing the path
   * @param buffer
   * @return
   */
  virtual int Open(const char *path, void *buffer) { return 0; };

  /**
   * check if the image is Animated
   * @return true if Animated
   */
  virtual bool Animated() { return false; };

  /**
   * get any errors from the underlying image decoder as a string
   * @return decoder error as null terminated string
   */
  virtual const char *GetLastError() = 0;
};
}

extern std::array<const char *, 4> extensionArray;
extern std::span<const char *> extensions;
image::Image *ImageFactory(image::screenResolution res, const char *path);