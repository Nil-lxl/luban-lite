# ZIP File System Library

## Overview

This library provides two approaches to handle ZIP files:

1. **ZIP File System**: Mount ZIP files or the block device where the ZIP data in as virtual file systems
2. **ZIP File Reader**: Directly read ZIP file contents

## ZIP File System

### API Functions
```c
int zipfs_mount(const char *zip_path, const char *mount_point);
int zipfs_unmount(const char *mount_point);
```

### Important Notes
- Not thread-safe, requires external locking in multi-threaded environments
- Mount point must be an empty directory
- Read-only access

## ZIP File Reader

### API Functions
```c
struct aic_zip_reader *aic_zip_reader_open(const char *source);
int aic_zip_reader_read(struct aic_zip_reader *reader,
                       struct aic_file_info file_info,
                       void *buffer);
void aic_zip_reader_close(struct aic_zip_reader *reader);
```

### Important Notes
- After opening a ZIP reader, you must manually call aic_zip_reader_close to close it
- You can adjust the read buffer size by modifying the READ_BUF_SIZE macro in zip_reader.h
- Large files are automatically streamed internally, but note that the provided buffer must be greater than or equal to the file size
- Encrypted ZIP and ZIP64 formats are not supported

## Demo Programs

The project includes two demonstration programs:
- `zip_fs_demo.c`: ZIP file system usage example
- `zip_reader_demo.c`: ZIP file reader usage example

## Dependencies

- RT-Thread: Real-time operating system
