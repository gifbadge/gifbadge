#include "jpeg.h"
#include <cassert>
#include <string>
#include "tjpgd.h"
#ifdef __XTENSA__
#include "esp_heap_caps.h"
#endif

JPEG::~JPEG() {
    printf("JPEG DELETED\n");
    free((void *) workbuf);
    fclose(jpguser.infile);
}

static const char *jresult_to_str(JRESULT ret){
    switch (ret) {
        case JDR_OK:
            return "Succeeded";
        case JDR_INTR:
            return "Interrupted by output function";
        case JDR_INP:
            return "Device error or wrong termination of input stream";
        case JDR_MEM1:
            return "Insufficient memory pool for the image";
        case JDR_MEM2:
            return "Insufficient stream input buffer";
        case JDR_PAR:
            return "Parameter error";
        case JDR_FMT1:
            return "Data format error (may be broken data)";
        case JDR_FMT2:
            return "Right format but not supported";
        case JDR_FMT3:
            return "Not supported JPEG standard";
    }
    return "";
}

frameReturn JPEG::loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) {
    jpguser.outBuf = outBuf;
    rewind(jpguser.infile);
    JRESULT ret = jd_prepare(&_dec, jpeg_decode_in_cb, workbuf, JPEG_WORK_BUF_SIZE, &jpguser);
    if (ret != JDR_OK) {
      lastErr = jresult_to_str(ret);
      return {frameStatus::ERROR, 0};
    }
    ret = jd_decomp(&_dec, jpeg_decode_out_cb, 0);
    if(ret != 0){
        lastErr = jresult_to_str(ret);
      return {frameStatus::ERROR, 0};
    }
  return {frameStatus::END, 60*60000}; //Only loop once an hour
}

std::pair<int16_t, int16_t> JPEG::size() {
    return {_dec.width, _dec.height};
}

size_t JPEG::jpeg_decode_in_cb(JDEC *dec, uint8_t *buff, size_t nbyte) {
    size_t to_read = nbyte;
    auto *in = (JPGuser *) dec->device;

    if (buff) {
        if (ftell(in->infile) + to_read > in->size) {
            to_read = in->size - ftell(in->infile);
        }

        /* Copy data from JPEG image */
        fread(buff, 1, to_read, in->infile);
    } else {
        /* Skip data */
        fseek(in->infile, to_read, SEEK_CUR);
    }

    return to_read;
}


jpeg_decode_out_t JPEG::jpeg_decode_out_cb(JDEC *dec, void *bitmap, JRECT *rect) {
    //TODO: Handle non-native sized jpegs
    assert(dec != nullptr);

    auto *cfg = (JPGuser *) dec->device;
    assert(bitmap != nullptr);
    assert(rect != nullptr);

    /* Copy decoded image data to output buffer */
    auto *in = (uint8_t *) bitmap;
    uint32_t line = dec->width;
    auto *dst = (uint8_t *) cfg->outBuf;
    for (int y = rect->top; y <= rect->bottom; y++) {
        for (int x = rect->left; x <= rect->right; x++) {
            for (int b = 0; b < ESP_JPEG_COLOR_BYTES; b++) {
                dst[(y * line * ESP_JPEG_COLOR_BYTES) + x * ESP_JPEG_COLOR_BYTES + b] = in[ESP_JPEG_COLOR_BYTES - b -
                                                                                           1];
            }
            in += ESP_JPEG_COLOR_BYTES;
        }
    }

    return 1;
}

Image *JPEG::create() {
    return new JPEG();
}


int JPEG::open(const char *path, void *buffer) {
    jpguser.infile = fopen(path, "r");
    fseek(jpguser.infile, 0, SEEK_END);
    jpguser.size = ftell(jpguser.infile);
    rewind(jpguser.infile);
    printf("JPEG Size %zu\n", jpguser.size);
#ifdef __XTENSA__
    workbuf = (uint8_t *) heap_caps_malloc(JPEG_WORK_BUF_SIZE, MALLOC_CAP_DEFAULT);
#else
    workbuf = (uint8_t *) malloc(JPEG_WORK_BUF_SIZE);
#endif

    if (workbuf == nullptr) {
        lastErr = "Failed to allocate memory";
        return -1;
    }

    JRESULT ret = jd_prepare(&_dec, jpeg_decode_in_cb, workbuf, JPEG_WORK_BUF_SIZE, &jpguser);
    if (ret != JDR_OK) {
        lastErr = jresult_to_str(ret);
        return -1;
    }

    return 0;
}

const char * JPEG::getLastError() {
    return lastErr;
}
