/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <dirent.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_LEN 128


#define JOIN_PATH(a, b, c) snprintf(a,  MAX_FILE_LEN, "%s/%s", b, c)

typedef struct {
  long file_index[255];
  DIR *dirptr;
  int count;
  int index;
} DIR_SORTED;

int opendir_sorted(DIR_SORTED *, const char *dirname, int(*validator)(const char *, const char *));

struct dirent *readdir_sorted(DIR_SORTED *);

void seekdir_sorted(DIR_SORTED *dirp, int index);

void closedir_sorted(DIR_SORTED *dirp);

void rewinddir_sorted(DIR_SORTED *dirp);

int directory_get_position(DIR_SORTED *dirp, const char *file);

const char * directory_get_increment(DIR_SORTED *dirp, int pos, int increment);

void directory_print(DIR_SORTED *dirp);

int is_directory(const char* path);

int compare_path(char *a, char *b);

#ifdef __cplusplus
}
#endif