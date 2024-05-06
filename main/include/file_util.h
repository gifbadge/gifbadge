#pragma once
#include <vector>
#include <string>
#include <dirent.h>
#include <ff.h>

std::vector<std::string> list_folders(const std::string &path);
std::vector<std::string> list_directory(const std::string &path);
bool valid_file(const char *path);
char *dirname (char *path);
bool is_dir(const char *path);