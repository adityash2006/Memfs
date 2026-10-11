#define FUSE_USE_VERSION 31
#include "../include/fuse.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

static const char *HELLO_PATH = "/hello.txt";
static const char *HELLO_CONTENT = "Hello World!\n";

/**
 * fs_getattr:
 * FUSE calls this to fetch file/directory metadata (permissions, type, size, nlinks).
 * Corresponds to standard stat() / lstat() system calls.
 */
static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi; 

    printf("[DEBUG] getattr called for path: %s\n", path);
    memset(st, 0, sizeof(struct stat));

    // Case 1: Root directory "/"
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755; // Directory with rwxr-xr-x
        st->st_nlink = 2;             // '.' and directory entry
        return 0;
    }

    // Case 2: Virtual file "/hello.txt"
    if (strcmp(path, HELLO_PATH) == 0) {
        st->st_mode = S_IFREG | 0644;        // Regular file with rw-r--r--
        st->st_nlink = 1;                    // Single link
        st->st_size = strlen(HELLO_CONTENT); // Exact payload size in bytes
        return 0;
    }
    printf("this file doesnt exist in the folder how am i supposed to read it bruh ?");


    // Path does not exist in memfs
    return -ENOENT;
}

/**
 * fs_readdir:
 * FUSE calls this to list entries inside a directory.
 * Corresponds to opendir() / readdir() / ls commands.
 */
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; 
    (void) fi; 
    (void) flags; 

    printf("[DEBUG] readdir called for path: %s\n", path);

    // memfs currently only supports the root directory "/"
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    // Standard POSIX directory entries
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    filler(buf, "hello.txt", NULL, 0, 0);

    return 0; 
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    (void) fi;

    printf("[DEBUG] read called for path: %s, size: %zu, offset: %ld\n", 
           path, size, (long)offset);

    // Ensure requested file exists
    if (strcmp(path, HELLO_PATH) != 0) {
        return -ENOENT;
    }

    size_t len = strlen(HELLO_CONTENT);

    // If offset requested is at or past end-of-file, return 0 (EOF)
    if ((size_t)offset >= len) {
        return 0;
    }

    // If requested read spans beyond available content, clamp size
    if (offset + size > len) {
        size = len - offset;
    }

    // Copy the slice from the source string into the kernel buffer
    memcpy(buf, HELLO_CONTENT + offset, size);

   
    return (int)size;
}

static struct fuse_operations my_ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read    = fs_read,
};

int main(int argc, char *argv[]) {
    printf("Starting memfs (Phase 2)... Mounting filesystem now!\n");
    return fuse_main(argc, argv, &my_ops, NULL);
}