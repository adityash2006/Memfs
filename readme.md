# memfs: An In-Memory FUSE Filesystem in C

`memfs` is a custom, single-threaded Unix filesystem implemented in userspace using the **FUSE 3 (Filesystem in Userspace)** framework and the C programming language. Rather than reading and writing raw blocks to a physical disk or SSD, `memfs` translates POSIX filesystem operations directly into in-memory data structures residing in RAM.

---



### 1. Project Description
Standard modern operating system kernels manage physical storage via native filesystem drivers (e.g., ext4, Btrfs, XFS) operating in protected kernel space. While highly performant, kernel-level filesystem development carries significant risk: minor bugs or pointer miscalculations can trigger kernel panics.

`memfs` addresses this challenge by implementing an educational, functional filesystem in userspace using the Linux FUSE API. The filesystem acts as a virtual storage target mounted at an arbitrary directory (e.g., `mnt/`). When standard Linux utilities (`ls`, `cat`, `echo`, `mkdir`, `touch`) interact with the mount point, the Linux Virtual File System (VFS) routes requests through `/dev/fuse` to the `memfs` userspace process. The program executes the corresponding callbacks in memory and returns standard POSIX status codes back to the OS.

### 2. Goals & Learning Objectives
* **POSIX & VFS Interface:** Master how the Linux Virtual File System maps human-readable terminal commands to POSIX file system hooks (`getattr`, `readdir`, `read`, `write`, `create`, `mkdir`).
* **Systems Programming in C:** Gain practical experience with low-level memory layout, pointer mechanics, fixed-size buffering, and error propagation (`errno`).
* **Userspace Filesystem Architecture:** Understand the interaction between the kernel-space FUSE driver and userspace callback loops.


### 3. System Specifications

#### Architectural Limits
To ensure memory safety and minimize fragmentation during initial phases, `memfs` enforces fixed-size parameters:

| Parameter | Specification | Notes |
|---|---|---|
| **Max Tracked Nodes** | 64 entries | Total capacity for regular files and directories. |
| **Max Path Length** | 256 bytes | Maximum absolute path length (e.g., `/docs/notes.txt`). |
| **Max File Capacity** | 64 Kilobytes (65,536 bytes) | Fixed byte buffer allocated per regular file node. |
| **FUSE API Target** | FUSE 3.x (`FUSE_USE_VERSION 31`) | Target framework for modern Linux environments. |

#### Core In-Memory Data Structures
Every file and directory in `memfs` is managed via an `fs_node_t` abstraction:




```c

#define MAX_NODES 64
#define MAX_PATH_LEN 256
#define MAX_FILE_SIZE 65536

typedef struct {
    char path[MAX_PATH_LEN];         // Absolute path starting with "/"
    char data[MAX_FILE_SIZE];        // File payload buffer
    size_t size;                     // Current file size in bytes
    mode_t mode;                     // File type & permissions (S_IFREG / S_IFDIR)
    time_t last_modified;            // Last modification timestamp
} fs_node_t;

static fs_node_t file_system[MAX_NODES];


```
### 4. Design / System Workflow

graph TD 
    %% Define Color Styles
    classDef terminal fill:#E6F2FF,stroke:#4A90E2,stroke-width:2px;
    classDef kernel fill:#FFF0F5,stroke:#D0021B,stroke-width:2px;
    classDef myProgram fill:#E2F9E5,stroke:#1EA83C,stroke-width:2px;
    classDef ramStorage fill:#FFFDF0,stroke:#F5A623,stroke-width:2px;

    %% 1. User Application Layer
    subgraph UserSpaceApp ["User Space (Applications)"]
        A["Terminal Command <br/> <i>(e.g., ls mnt/ OR cat mnt/hello.txt)</i>"]
    end

    %% 2. Kernel space layer
    subgraph KernelSpace ["Kernel Space (Protected OS Core)"]
        B["1. Virtual File System (VFS) <br/> <i>Intercepts POSIX calls (open, read, write)</i>"]
        C["2. FUSE Kernel Driver <br/> <i>Redirects commands to userspace mount</i>"]
    end

    %% 3. Your C Program Layer
    subgraph MyFilesystem ["User Space (Your memfs Program)"]
        D["3. /dev/fuse Channel <br/> <i>Your C app reads request packets</i>"]
        E["4. Callback Switchboard <br/> <i>fuse_operations matches call to C function</i>"]
        
        subgraph MyCallbacks ["Your Custom Callback Functions"]
            F["fs_getattr() <br/> <i>Metadata</i>"]
            G["fs_readdir() <br/> <i>Directory List</i>"]
            H["fs_read() <br/> <i>Read Data</i>"]
            I["fs_write() <br/> <i>Write Data</i>"]
        end
        
        J[("5. In-Memory Array <br/> file_system[64] in RAM")]
    end

    %% Apply Styles to Nodes and Subgraphs (No Semicolons)
    class UserSpaceApp,A terminal
    class KernelSpace,B,C kernel
    class MyFilesystem,D,E myProgram
    class MyCallbacks,F,G,H,I,J ramStorage

    %% --- REQUEST PATH ---
    A ==>|"1. Executes Call"| B
    B ==>|"2. Matches Mount"| C
    C ==>|"3. Serializes via /dev/fuse"| D
    D ==>|"4. Unpacks & Dispatches"| E
    E --> F
    E --> G
    E --> H
    E --> I
    F <==>|"Inspects"| J
    G <==>|"Loops over"| J
    H <==>|"Copies from"| J
    I <==>|"Writes to"| J

    %% --- RESPONSE PATH ---
    J -.->|"Raw buffer bytes / status"| E
    E -.-> D
    D -.->|"Returned up to kernel"| C
    C -.-> B
    B -.->|"Terminal prints output"| A


