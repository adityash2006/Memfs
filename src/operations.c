#include "../include/memfs.h"


fs_node_t file_system[MAX_NODES];
int total_nodes = 0;

int find_node(const char *path) {
    for (int i = 0; i < total_nodes; i++) {
        if (strcmp(file_system[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}


int add_node(const char *path, mode_t mode) {
    if (total_nodes >= MAX_NODES) {
        return -ENOSPC; // No space left on device
    }
    if (strlen(path) >= MAX_PATH_LEN) {
        return -EINVAL; // File name too long
    }

    int idx = total_nodes;
    memset(&file_system[idx], 0, sizeof(fs_node_t));
    strncpy(file_system[idx].path, path, MAX_PATH_LEN - 1);
    file_system[idx].mode = mode;
    file_system[idx].size = 0;
    file_system[idx].last_modified = time(NULL);

    total_nodes++;
    return idx;
}

/* FUSE Callbacks  */

/**
 * fs_getattr:
 * Inspects node attributes for stat() / ls.
 */
static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi;
    memset(st, 0, sizeof(struct stat));

    // Root directory check
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        st->st_mtime = time(NULL);
        return 0;
    }

    // Search dynamic array
    int idx = find_node(path);
    if (idx != -1) {
        st->st_mode = file_system[idx].mode;
        st->st_nlink = 1;
        st->st_size = file_system[idx].size;
        st->st_mtime = file_system[idx].last_modified;
        return 0;
    }

    return -ENOENT;
}

/**
 * fs_readdir:
 * Iterates through file_system[] and returns directory entries.
 */
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    // List all dynamic entries registered under root
    for (int i = 0; i < total_nodes; i++) {
        // Strip leading '/' for directory filler
        const char *name = file_system[i].path;
        if (name[0] == '/') {
            name++;
        }
        filler(buf, name, NULL, 0, 0);
    }

    return 0;
}

/**
 * fs_create:
 * Triggered on file creation (e.g. `touch mnt/test.txt` or opening in O_CREAT mode).
 */
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    printf("[DEBUG] create called for path: %s\n", path);

    if (find_node(path) != -1) {
        return -EEXIST; // File already exists
    }

    int res = add_node(path, S_IFREG | mode);
    if (res < 0) {
        return res;
    }
    return 0;
}

/**
 * fs_read:
 * Reads byte stream from file_system[idx].data.
 */
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    (void) fi;
    int idx = find_node(path);
    if (idx == -1) {
        return -ENOENT;
    }

    fs_node_t *node = &file_system[idx];

    if ((size_t)offset >= node->size) {
        return 0; // EOF
    }

    if (offset + size > node->size) {
        size = node->size - offset;
    }

    memcpy(buf, node->data + offset, size);
    return (int)size;
}

/**
 * fs_write:
 * Writes incoming bytes from user space into node memory.
 * Handles append, overwrite, and boundary checks against MAX_FILE_SIZE.
 */
static int fs_write(const char *path, const char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    (void) fi;
    printf("[DEBUG] write called for path: %s, size: %zu, offset: %ld\n",
           path, size, (long)offset);

    int idx = find_node(path);
    if (idx == -1) {
        return -ENOENT;
    }

    fs_node_t *node = &file_system[idx];

    // Check if write starts beyond max capacity
    if ((size_t)offset >= MAX_FILE_SIZE) {
        return -EFBIG; // File too large
    }

    // Clamp size if write exceeds remaining buffer in slot
    if (offset + size > MAX_FILE_SIZE) {
        size = MAX_FILE_SIZE - offset;
    }

    // Copy stream bytes into RAM buffer
    memcpy(node->data + offset, buf, size);

    // Update node size if written beyond previous file length
    if (offset + size > node->size) {
        node->size = offset + size;
    }

    node->last_modified = time(NULL);
    return (int)size;
}

/**
 * fs_truncate:
 * Called when opening a file with O_TRUNC (e.g. `> file` or `echo "..." > file`).
 * Resets the file size.
 */
static int fs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) fi;
    printf("[DEBUG] truncate called for path: %s to size: %ld\n", path, (long)size);

    int idx = find_node(path);
    if (idx == -1) {
        return -ENOENT;
    }

    if ((size_t)size > MAX_FILE_SIZE) {
        return -EFBIG;
    }

    fs_node_t *node = &file_system[idx];
    node->size = size;
    node->last_modified = time(NULL);

    return 0;
}

/**
 * fs_utimens:
 * Called by touch and utimensat() system calls to update file access/modification timestamps.
 * Resolves the "touch: setting times of '...': Function not implemented" warning.
 */
 static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    (void) fi;
    printf("[DEBUG] utimens called for path: %s\n", path);

    int idx = find_node(path);
    if (idx == -1) {
        return -ENOENT;
    }

    // tv[1] represents modification time (st_mtime)
    if (tv != NULL) {
        file_system[idx].last_modified = tv[1].tv_sec;
    } else {
        file_system[idx].last_modified = time(NULL);
    }

    return 0;
}

/* ========================================================================= */
/* Registered Operations Table                                               */
/* ========================================================================= */
struct fuse_operations memfs_operations = {
    .getattr  = fs_getattr,
    .readdir  = fs_readdir,
    .create   = fs_create,
    .read     = fs_read,
    .write    = fs_write,
    .truncate = fs_truncate,
    .utimens  = fs_utimens,
};
