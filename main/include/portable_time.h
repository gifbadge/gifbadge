#pragma once

#ifdef ESP_PLATFORM
#include <esp_timer.h>
#define millis() esp_timer_get_time()/1000
#else
#ifdef __cplusplus
extern "C"
{
#endif
int64_t get_millis();
#ifdef __cplusplus
}
#endif
#define millis() get_millis()
#endif