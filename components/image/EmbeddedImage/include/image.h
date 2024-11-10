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
class Image {

 public:
  Image() = default;

  virtual ~Image() = default;

  /**
   *
   * @param outBuf the buffer to write too
   * @param x offset in x direction
   * @param y offset in y direction
   * @param width width of the display
   * @return
   */
  virtual frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) { return {frameStatus::OK, 0}; };

  /**
   * Get the size of the image. Can only be called after image is opened with open()
   * @return x, y pair representing the width and height of the image
   */
  virtual std::pair<int16_t, int16_t> size() { return {0, 0}; };

  /**
   * Open an image from a path.
   * if buffer is provided, it will be used as working memory for image decoding
   * @param path null terminated string containing the path
   * @param buffer
   * @return
   */
  virtual int open(const char *path, void *buffer) { return 0; };

  /**
   * check if the image is animated
   * @return true if animated
   */
  virtual bool animated() { return false; };

  /**
   * get any errors from the underlying image decoder as a string
   * @return decoder error as null terminated string
   */
  virtual const char *getLastError() = 0;
};
}

extern std::array<const char *, 4> extensionArray;
extern std::span<const char *> extensions;
image::Image *ImageFactory(const char *path);