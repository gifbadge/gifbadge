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

#define BUFFER_SIZE (2*1024*1024) // 1 Megabyte
#define BUFFER_CHUNK (4096)
#define ESP_PLATFORM

TaskHandle_t file_buffer_task = nullptr;

QueueHandle_t FileQueue;
SemaphoreHandle_t readSemaphore;
SemaphoreHandle_t writeSemaphore;
SemaphoreHandle_t lockSemaphore;


struct circular_buf_t {
  uint8_t *data = nullptr;
  volatile uint8_t * write_pos = nullptr;
  volatile uint8_t * read_pos = nullptr;
  int32_t file_pos = 0;
  size_t file_size = 0;
  int fd = 0;
  char open_file[255] = "";
  bool start_valid = false;
  volatile uint8_t * start_pos = nullptr;
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

static size_t cbuffer_put_file(circular_buf_t *buffer, size_t size) {
  size_t count = 0;

  if (buffer->write_pos >= buffer->read_pos) {
    size_t end_free_segment = (buffer->data + BUFFER_SIZE) - buffer->write_pos + (buffer->read_pos == buffer->data ? 0 : 1);

    if (end_free_segment >= size)
    {
      count = read(buffer->fd, const_cast<uint8_t *>(buffer->write_pos), size);
      buffer->write_pos += size;
    } else {
      count = read(buffer->fd, const_cast<uint8_t *>(buffer->write_pos), end_free_segment);
      count += read(buffer->fd, buffer->data, size - end_free_segment);
      buffer->write_pos = buffer->data + size - end_free_segment;
    }
  } else {
    count = read(buffer->fd, const_cast<uint8_t *>(buffer->write_pos), size);
    buffer->write_pos += size;
  }

  return count;
}

static int32_t cbuffer_read(circular_buf_t *buffer, uint8_t *dest, int32_t size) {
  if (buffer->write_pos >= buffer->read_pos) {
    memcpy(dest, const_cast<uint8_t *>(buffer->read_pos), size);
  } else {
    size_t end_data_segment = buffer->data + BUFFER_SIZE - buffer->read_pos + (buffer->read_pos == buffer->data ? 0 : 1);
    if (end_data_segment >= size)
    {
      memcpy(dest, const_cast<uint8_t *>(buffer->read_pos), size);
    } else {
      memcpy(dest, const_cast<uint8_t *>(buffer->read_pos), end_data_segment);
      memcpy(dest + end_data_segment, buffer->data, size - end_data_segment);
    }
  }
  buffer->read_pos += size;
  if (buffer->read_pos > buffer->data + BUFFER_SIZE)
    buffer->read_pos -= BUFFER_SIZE + 1;
  buffer->file_pos += size;
  return size;
}

void cbuffer_open(circular_buf_t *buffer, char *path) {
  if (buffer->fd > 0) {
    close(buffer->fd);
    buffer->fd = 0;
  }
  cbuffer_reset(&cbuffer);
  if (strlen(path) > 0) {
    buffer->fd = open(path, O_RDONLY);
    if (buffer->fd == -1) {
      printf("Couldn't open file %s: %s\n", path, strerror(errno));
    } else {
      struct stat buf{};
      fstat(buffer->fd, &buf);
      buffer->file_size = buf.st_size;
      printf("Opened %s size %d\n", path, buffer->file_size);
    }
  }
}

[[noreturn]] void FileBufferTask(void *) {
  printf("Starting FileBuffer task");
  char path[255] = "";
  FileQueue = xQueueCreate(1, 255);
  readSemaphore = xSemaphoreCreateBinary();
  writeSemaphore = xSemaphoreCreateBinary();
  lockSemaphore = xSemaphoreCreateMutex();
  cbuffer_init(&cbuffer, BUFFER_SIZE);

  BaseType_t option = errQUEUE_EMPTY;
  TickType_t delay;
  while (true) {
    if (cbuffer_get_free(&cbuffer) - BUFFER_CHUNK * 2 >= BUFFER_CHUNK && cbuffer.fd > 0) {
      delay = 0;
    } else {
      delay = 50 / portTICK_PERIOD_MS;
    }
    option = xQueueReceive(FileQueue, &path, delay);
    if (option == pdPASS) {
      cbuffer_open(&cbuffer, path);
      xSemaphoreTake(readSemaphore, 0);
      xSemaphoreGive(writeSemaphore);
      xSemaphoreGive(lockSemaphore);
    }
    if (cbuffer.fd > 0) {
      if ((cbuffer_get_free(&cbuffer) - (BUFFER_CHUNK * 2)) >= BUFFER_CHUNK) {
        size_t count = cbuffer_put_file(&cbuffer, BUFFER_CHUNK);
        if (count < BUFFER_CHUNK) {
          if (cbuffer.file_size > BUFFER_SIZE) {
            cbuffer.start_pos = cbuffer.write_pos;
            lseek(cbuffer.fd, 0, SEEK_SET);
            cbuffer_put_file(&cbuffer, BUFFER_CHUNK);
            cbuffer.start_valid = true;
          } else {
            xQueuePeek(FileQueue, &path, portMAX_DELAY);
          }
        }
        xSemaphoreGive(readSemaphore);
      } else {
        xSemaphoreTake(writeSemaphore, 50 / portTICK_PERIOD_MS);
      }
    }
  }
}

void filebuffer_open(const char *path) {
  strcpy(cbuffer.open_file, path);
  xSemaphoreTake(lockSemaphore, portMAX_DELAY);
  xQueueSend(FileQueue, &cbuffer.open_file, 0);
}

void filebuffer_close() {
  constexpr char path[255] = "";
  xQueueSend(FileQueue, &path, 0);
}

int32_t filebuffer_read(uint8_t *pBuf, const int32_t iLen) {
  xSemaphoreTake(lockSemaphore, portMAX_DELAY);
  while (cbuffer_get_avail(&cbuffer) < iLen) {
    xSemaphoreTake(readSemaphore, 2 / portTICK_PERIOD_MS);
  }
  xSemaphoreGive(writeSemaphore);
  xSemaphoreGive(lockSemaphore);
  return cbuffer_read(&cbuffer, pBuf, iLen);
}

void filebuffer_seek(int32_t pos) {
  int32_t new_pos = pos - cbuffer.file_pos;
  if ((cbuffer.file_size > BUFFER_SIZE) && new_pos < -BUFFER_CHUNK*2) {
    if (cbuffer.start_valid) {
      cbuffer.read_pos = cbuffer.start_pos;
    } else {
      filebuffer_open(cbuffer.open_file);
    }
    cbuffer.start_valid = false;
    cbuffer.file_pos = 0;
  }
  else {
    cbuffer.read_pos += new_pos;
    cbuffer.file_pos += new_pos;
  }
}
