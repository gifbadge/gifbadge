//
// Created by gifbadge on 09/05/24.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "include/directory.h"

#if defined(__GLIBC__) || (defined (__FreeBSD__) && defined(qsort_r))
static int cmpfunc (const void * a, const void * b, void *arg) {
#else
static int cmpfunc (void *arg, const void * a, const void * b) {
#endif
  DIR* dir = (DIR *)arg;
  struct dirent *de;
  char file_name_a[256];
  char file_name_b[256];

  seekdir(dir, *(long *)a);
  de = readdir(dir);
  strncpy(file_name_a, de->d_name, sizeof(file_name_a));
  seekdir(dir, *(long *)b);
  de = readdir(dir);
  strncpy(file_name_b, de->d_name, sizeof(file_name_b));
  return strcmp(file_name_a, file_name_b);
}

int opendir_sorted(DIR_SORTED *dirp, const char *dirname, int(*validator)(const char *, const char *)) {
  dirp->dirptr = opendir(dirname);
  if(!dirp->dirptr){
    return 0;
  }
  dirp->count = 0;
  long tmp_ptr;
  struct dirent *de;
  while (1) {
    dirp->file_index[dirp->count] = telldir(dirp->dirptr);
    de = readdir(dirp->dirptr);
    if(!de){
      break;
    }
    if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) {
      // Throw away . and ..
      continue;
    }
    if(validator){
      if (!validator(dirname, de->d_name)) {
        continue;
      }
    }
    dirp->count++;
  }
  dirp->count = dirp->count - 1;
  dirp->index = 0;
#if defined(__GLIBC__) || (defined (__FreeBSD__) && defined(qsort_r))
  qsort_r(dirp->file_index, dirp->count + 1, sizeof(long), cmpfunc, dirp->dirptr);
#else
  qsort_r(dirp->file_index, dirp->count, sizeof(long), dirp->dirptr, cmpfunc);
#endif
  rewinddir_sorted(dirp);
  return 1;
}

struct dirent *readdir_sorted(DIR_SORTED *dirp) {
  if (dirp->index > dirp->count || dirp->count < 0) {
    return NULL;
  }
  seekdir(dirp->dirptr, dirp->file_index[dirp->index]);
  struct dirent *de = readdir(dirp->dirptr);
  dirp->index++;
  return de;
}

void seekdir_sorted(DIR_SORTED *dirp, int index) {
  seekdir(dirp->dirptr, dirp->file_index[index]);
  dirp->index = index;
}

void closedir_sorted(DIR_SORTED *dirp) {
  if(dirp->dirptr) {
    closedir(dirp->dirptr);
  }
}

void rewinddir_sorted(DIR_SORTED *dirp){
  dirp->index = 0;
}

int directory_get_position(DIR_SORTED *dirp, const char *file){
  struct dirent *de;

  rewinddir_sorted(dirp);

  while(1){
       de = readdir_sorted(dirp);
      if (!de) {
        return -1;
      }
      if(strcmp(de->d_name, file) == 0){
          return dirp->index-1;
      }
    }
}

const char * directory_get_increment(DIR_SORTED *dirp, int pos, int increment){
  struct dirent *de;
  if (dirp->count < 0) {
    return NULL;
  }
  while(1){
    if(pos+(increment) > dirp->count){
      pos = 0;
    } else if(pos+(increment) < 0){
      pos = dirp->count;
    } else {
      pos = pos+(increment);
    }
    seekdir_sorted(dirp, pos);
    de = readdir_sorted(dirp);
    if(de){
      return de->d_name;
    }
  }
}

//void directory_print(DIR_SORTED *dirp){
//#ifdef ESP_PLATFORM
//#include <esp_log.h>
//  rewinddir_sorted(dirp);
//  struct dirent *de;
//  while((de = readdir_sorted(dirp))){
//    ESP_LOGI("directory.c", "%s", de->d_name);
//  }
//#else
//  rewinddir_sorted(dirp);
//  struct dirent *de;
//  while((de = readdir_sorted(dirp))){
//    printf("%s\n", de->d_name);
//  }
//#endif
//}

int is_directory(const char* path){
  struct stat buffer;
  stat(path, &buffer);
  return (buffer.st_mode & S_IFDIR) > 0;
}

int compare_path(char *a, char *b){
  if(a[strlen(a)-1] == '/'){
    a[strlen(a)-1] = '\0';
  }
  if(b[strlen(b)-1] == '/'){
    b[strlen(b)-1] = '\0';
  }
  return strcmp(a, b);
}
