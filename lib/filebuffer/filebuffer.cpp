#include "filebuffer.h"

#include <stdatomic.h>
#include <cstdio>
#include <cstring>
#include <esp_heap_caps.h>
#include <mutex>
#include <sys/stat.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <freertos/semphr.h>

#define BUFFER_SIZE (1*1024*1024) // 1 Megabyte
#define BUFFER_CHUNK 4096
#define ESP_PLATFORM

QueueHandle_t FileQueue;
SemaphoreHandle_t readSemaphore;
SemaphoreHandle_t writeSemaphore;
SemaphoreHandle_t lockSemaphore;
SemaphoreHandle_t s1;
SemaphoreHandle_t s2;

struct circular_buf_t {
  uint8_t *data;
  std::atomic<uint8_t *> write_pos;
  std::atomic<uint8_t *> read_pos;
  int32_t file_pos;
  char open_file[255] = "";
};

circular_buf_t cbuffer;

static size_t cbuffer_get_free(const circular_buf_t *buffer) {
  if (buffer->write_pos == buffer->read_pos) {
    return BUFFER_SIZE;
  }
  if (buffer->read_pos >= buffer->write_pos) {
    return buffer->read_pos - buffer->write_pos;
  }
  return (buffer->read_pos + BUFFER_SIZE + 1) - buffer->write_pos;
}

static size_t cbuffer_get_avail(const circular_buf_t *buffer) {
  return BUFFER_SIZE - cbuffer_get_free(buffer);
}

static void cbuffer_reset(circular_buf_t *buffer) {
  buffer->write_pos = buffer->data;
  buffer->read_pos = buffer->data;
  buffer->file_pos = 0;
}

static void cbuffer_init(circular_buf_t *buffer, size_t size) {
  buffer->data = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM));
  cbuffer_reset(buffer);
}

static size_t cbuffer_put_file(circular_buf_t *buffer, int fd, size_t size) {
  size_t end_free_segment;
  if (buffer->write_pos >= buffer->read_pos) {
    end_free_segment = (buffer->data + BUFFER_SIZE) - buffer->write_pos
        + (buffer->read_pos == buffer->data ? 0 : 1);

    if (end_free_segment >= size) /* Simple case */
    {
      read(fd, buffer->write_pos, size);
      buffer->write_pos.fetch_add(size);
    } else {
      read(fd, buffer->write_pos, end_free_segment);
      read(fd, buffer->data, size - end_free_segment);
      buffer->write_pos = buffer->data + size - end_free_segment;
    }
  } else {
    read(fd, buffer->write_pos, size);
    buffer->write_pos.fetch_add(size);
  }
  // printf("Wrote %d bytes\n", size);

  return size;
}

static size_t cbuffer_read(circular_buf_t *buffer, uint8_t *dest, size_t size) {
  if (buffer->write_pos >= buffer->read_pos) {
    memcpy(dest, buffer->read_pos, size);
  } else {
    size_t end_data_segment = buffer->data + BUFFER_SIZE - buffer->read_pos
        + (buffer->read_pos == buffer->data ? 0 : 1);
    if (end_data_segment >= size) /* Simple case */
    {
      memcpy(dest, buffer->read_pos, size);
    } else {
      memcpy(dest, buffer->read_pos, end_data_segment);
      memcpy(dest + end_data_segment,
             buffer->data,
             size - end_data_segment);
    }
  }
  buffer->read_pos.fetch_add(size);
  if (buffer->read_pos > buffer->data + BUFFER_SIZE)
    buffer->read_pos -= BUFFER_SIZE + 1;
  buffer->file_pos += size;
  return size;
}

void FileBufferTask(void *) {
  printf("Starting FileBuffer task");
  int fd = 0;
  char path[255] = "";
  FileQueue = xQueueCreate(1, 255);
  readSemaphore = xSemaphoreCreateBinary();
  writeSemaphore = xSemaphoreCreateBinary();
  lockSemaphore = xSemaphoreCreateMutex();
  // s1 = xSemaphoreCreateBinary();
  // s2 = xSemaphoreCreateBinary();
  xSemaphoreGive(writeSemaphore);
  cbuffer_init(&cbuffer, BUFFER_SIZE);

  BaseType_t option = errQUEUE_EMPTY;
  TickType_t delay;
  while (true) {
    if ((cbuffer_get_free(&cbuffer) - 1000) >= BUFFER_SIZE * 0.8 && fd != 0) {
      delay = 0;
    } else {
      delay = 50 / portTICK_PERIOD_MS;
    }
    option = xQueueReceive(FileQueue, &path, delay);
    if (option == pdPASS) {
      // printf("Received Path %s\n", path);
      if (fd > 0) {
        close(fd);
        fd = 0;
      }
      cbuffer_reset(&cbuffer);
      if (strlen(path) > 0) {
        fd = open(path, O_RDONLY);
        if (fd == -1) {
          printf("Couldn't open file %s: %s\n", path, strerror(errno));
        }
      }
      xSemaphoreTake(readSemaphore, 0);
      xSemaphoreGive(lockSemaphore);
    }
    if (fd > 0) {
      if ((cbuffer_get_free(&cbuffer) - (BUFFER_CHUNK * 2)) >= BUFFER_CHUNK) {
        cbuffer_put_file(&cbuffer, fd, BUFFER_CHUNK);
        xSemaphoreGive(readSemaphore);
      } else {
        printf("Blocked on Write\n");
        xSemaphoreTake(writeSemaphore, portMAX_DELAY);
      }
    }
  }
}

void openFile(const char *path) {
  strcpy(cbuffer.open_file, path);
  // printf("Opening file %s\n", cbuffer.open_file);
  xSemaphoreTake(lockSemaphore, portMAX_DELAY);
  xQueueSend(FileQueue, &cbuffer.open_file, 0);
}

void closeFile() {
  const char path[255] = "";
  xQueueSend(FileQueue, &path, 0);
}

size_t readFile(uint8_t *pBuf, int32_t iLen) {
  xSemaphoreTake(lockSemaphore, portMAX_DELAY);
  while (cbuffer_get_avail(&cbuffer) < iLen) {
    // printf("Blocked on Read, Available: %d bytes \n", cbuffer_get_free(&cbuffer));
    xSemaphoreTake(readSemaphore, portMAX_DELAY);
  }
  xSemaphoreGive(writeSemaphore);
  xSemaphoreGive(lockSemaphore);
  return cbuffer_read(&cbuffer, pBuf, iLen);
}

void seekFile(int32_t pos) {
  int32_t new_pos = pos - cbuffer.file_pos;
  if (new_pos < -BUFFER_CHUNK*2) {
    openFile(cbuffer.open_file);
  }
  else {
    cbuffer.read_pos += new_pos;
    cbuffer.file_pos += new_pos;
  }
}
