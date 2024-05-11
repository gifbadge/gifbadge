//
// Created by gifbadge on 09/05/24.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "include/directory.h"

static int cmpfunc (const void * a, const void * b, void *arg) {
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
  printf("cmpfunc: %s %s\n", file_name_a, file_name_b);
  return strcmp(file_name_a, file_name_b);
}

int opendir_sorted(DIR_SORTED *dirp, const char *dirname) {
  dirp->dirptr = opendir(dirname);
  if(!dirp->dirptr){
    return 0;
  }
  dirp->count = 0;
  while (1) {
    dirp->file_index[dirp->count] = telldir(dirp->dirptr);
    struct dirent *de = readdir(dirp->dirptr);
    if(!de){
      break;
    }
    dirp->count++;
  }
  dirp->index = 0;
  qsort_r(dirp->file_index, dirp->count, sizeof(long), cmpfunc, dirp->dirptr);
  return 1;
}

struct dirent *readdir_sorted(DIR_SORTED *dirp) {
  seekdir(dirp->dirptr, dirp->file_index[dirp->index]);
  dirp->index++;
  return readdir(dirp->dirptr);
}

void seekdir_sorted(DIR_SORTED *dirp, int index) {
  seekdir(dirp->dirptr, dirp->file_index[index]);
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
          return dirp->index;
      }
    }
}

const char * directory_get_next(DIR_SORTED *dirp, int pos){
  struct dirent *de;
  for(int c = 0; c < dirp->count; c++){
    seekdir_sorted(dirp, pos+1>dirp->count?0:pos+1);
    de = readdir_sorted(dirp);
    if(de){
      if(de->d_type != DT_DIR){
        return de->d_name;
      }
    }
    pos = pos+1>dirp->count?0:pos+1;
  }
  return "";
}

const char * directory_get_previous(DIR_SORTED *dirp, int pos){
  struct dirent *de;
  for(int c = 0; c < dirp->count; c++){
    seekdir_sorted(dirp, pos-1<0?dirp->count:pos-1);
    de = readdir_sorted(dirp);
    if(de){
      if(de->d_type != DT_DIR){
        return de->d_name;
      }
    }
    pos = pos-1<0?dirp->count:pos-1;
  }
  return "";
}

int is_directory(const char* path){
  struct stat buffer;
  stat(path, &buffer);
  return (buffer.st_mode & S_IFDIR);
}

int is_file(const char* path){
  struct stat buffer;
  stat(path, &buffer);
  return (buffer.st_mode & S_IFREG);
}

int compare_path(char *a, char *b){
  if(a[strlen(a)-1] == '/'){
    printf("a\n");
    a[strlen(a)-1] = '\0';
  }
  if(b[strlen(b)-1] == '/'){
    printf("b\n");
    b[strlen(b)-1] = '\0';
  }
  printf("a: %s b: %s\n", a, b);
  return strcmp(a, b);
}

//int main(void){
//  DIR_SORTED dir;
//  opendir_sorted(&dir, ".");
//  for(int c = 0; c < dir.count; c++){
//    struct dirent *de = readdir_sorted(&dir);
//    if(de) {
//      printf("%s\n", de->d_name);
//    }
//  }
//
//}
