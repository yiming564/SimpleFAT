# SimpleFAT: A Simple FAT32 Image Tool

本项目是 [xCore](https://github.com/404) 操作系统的辅助项目。

## 目前已经实现的功能

- 解析 RAW 格式的 FAT32 镜像。
- 遍历文件系统，读取文件内容。

## 计划开发的功能

- 增加写入文件系统的功能。
- 重构代码，统一接口。

## 已知的 Bug

- 在 Linux 系统下，FAT32 的文件名默认会使用 UTF-8 编码。（这是 Linux 的问题）

## Demo test:

1. 开发环境 Linux 系统，请安装 g++ make 等依赖软件。
2. 先运行脚本 `sandbox/init.sh` 创建镜像 `raw.img`，再运行脚本 `sandbox/sync.sh` 将 `sandbox/data/` 下的文件写入到镜像中。
3. `make run`。

不出意外的话，程序会遍历 `raw.img` 包含的文件系统中的所有文件。

## Miscs

本项目仍在开发早期，如果发现了什么 Bug 欢迎发 issue！