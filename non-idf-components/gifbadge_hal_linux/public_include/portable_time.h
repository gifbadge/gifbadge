#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
int64_t get_millis();
#ifdef __cplusplus
}
#endif
#define millis() get_millis()
