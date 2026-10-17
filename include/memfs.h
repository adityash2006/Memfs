#ifndef MEMFS_H
#define MEMFS_H

#define FUSE_USE_VERSION 31
#include "fuse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_NODES     64
#define MAX_PATH_LEN  256
#define MAX_FILE_SIZE 65536  // 64 KB per file


typedef struct {
    char path[MAX_PATH_LEN];
    char data[MAX_FILE_SIZE];
    size_t size;
    mode_t mode;             
    time_t last_modified;
} fs_node_t;


extern fs_node_t file_system[MAX_NODES];
extern int total_nodes;

int find_node(const char *path);
int add_node(const char *path, mode_t mode);
int remove_node(int idx);
int get_parent_dir(const char *path, char *parent_out);
int get_immediate_child_name(const char *parent_dir, const char *child_path, char *out_name);

extern struct fuse_operations memfs_operations;


#endif 