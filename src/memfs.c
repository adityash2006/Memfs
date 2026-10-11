#include "../include/memfs.h"

int main(int argc, char *argv[]) {
    printf("Starting memfs (Phase 3: Dynamic Storage)...\n");
    printf("Mounting filesystem with max capacity: %d files, %d KB/file\n", 
           MAX_NODES, MAX_FILE_SIZE / 1024);

    // control to FUSE using operations declared in operations.c
    return fuse_main(argc, argv, &memfs_operations, NULL);
}