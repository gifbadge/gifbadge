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
    /**
     *
     * @param path null terminated string containing the path
     */
    explicit Image(const char *path);

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
   * @param buffer
   * @return
   */
  virtual int Open(void *buffer) { return 0; };

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

  /**
   * Check if image is resizable
   * @return true if resizable
   */
  virtual bool resizable() { return false; };

    /**
     * 
     * @param x
     * @param y 
     * @return 
     */
    virtual int resize(int16_t x, int16_t y) { return -1; };

  protected:
  char _path[255];
};
}

extern std::array<const char *, 5> extensionArray;
extern std::span<const char *> extensions;
image::Image *ImageFactory(const char *path);

void CachedPath(const char *path, char *cachepath);