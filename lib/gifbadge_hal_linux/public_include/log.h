#pragma once

#include "console.h"
#define LOGI(tag, format, ...) console_print(tag, format, ##__VA_ARGS__)

