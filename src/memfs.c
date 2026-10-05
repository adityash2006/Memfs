#define FUSE_USE_VERSION 31
#include "../include/fuse.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi; 
    
    printf("[DEBUG] getattr called for path: %s\n", path);
    memset(st, 0, sizeof(struct stat));

  
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755; // This is a Directory with standard permissions (drwxr-xr-x)
        st->st_nlink = 2;             // Standard links for directories
        return 0;                     // 0 means Success
    }

    return -ENOENT;
}


static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags; 

    printf("[DEBUG] readdir called for path: %s\n", path);

    // We only allow reading the root directory "/"
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    // Every directory has to contain "." (current directory) and ".." (parent directory)
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    return 0; 
}

// Registering our callback functions with FUSE
static struct fuse_operations my_ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
};

int main(int argc, char *argv[]) {
    printf("Starting memfs... Mounting filesystem now!\n");
    // Hand over control to FUSE. It enters an infinite loop waiting for commands.
    return fuse_main(argc, argv, &my_ops, NULL);
}
