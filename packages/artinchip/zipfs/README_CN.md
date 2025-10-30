# ZIP文件系统库

## 功能概述

本库提供两种处理ZIP文件的方式：

1. **ZIP文件系统**：将ZIP文件或包含zip文件的块设备挂载为虚拟文件系统
2. **ZIP文件读取器**：直接读取ZIP文件内容

## ZIP文件系统

### 接口函数
```c
int zipfs_mount(const char *zip_path, const char *mount_point);
int zipfs_unmount(const char *mount_point);
```

### 注意事项
- 非线程安全，多线程环境需要外部加锁
- 挂载点必须是空的目录
- 只读访问

## ZIP文件读取器

### 接口函数
```c
struct aic_zip_reader *aic_zip_reader_open(const char *source);
int aic_zip_reader_read(struct aic_zip_reader *reader,
                       struct aic_file_info file_info,
                       void *buffer);
void aic_zip_reader_close(struct aic_zip_reader *reader);
```

### 注意事项
- 在打开ZIP读取器后，必须手动调用aic_zip_reader_close关闭读取器
- 可以在zip_reader.h中通过修改宏READ_BUF_SIZE调整读取缓冲区大小
- 读取大文件时内部会自动流式，但是要注意提供的buffer要大于或等于文件大小
- 不支持加密ZIP和ZIP64格式

## 示例程序

项目包含两个演示程序：
- `zip_fs_demo.c`: ZIP文件系统使用示例
- `zip_reader_demo.c`: ZIP文件读取器使用示例

## 依赖

- RT-Thread: 实时操作系统
