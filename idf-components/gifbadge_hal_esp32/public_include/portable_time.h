#pragma once

#include <esp_timer.h>
#define millis() esp_timer_get_time()/1000