#pragma once
#include <vector>
#include <string>
#include <dirent.h>
#include <ff.h>

bool valid_file(const char *path);
char *dirname (char *path);