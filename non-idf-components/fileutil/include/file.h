#pragma once

#include <span>

//#ifdef __cplusplus
//extern "C" {
//#endif

bool is_file(const char *path);
bool is_not_hidden(const char *path);
bool is_valid_extension(const char *path, std::span<const char *> extensions);
bool valid_image_file(const char *path, std::span<const char *> extensions);




//#ifdef __cplusplus
//}
//#endif