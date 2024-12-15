# SimpleFAT: A Simple FAT32 Image Tool

本项目是 [xCore](https://github.com/404) 操作系统的辅助项目。

## 目前已经实现的功能

- 解析 RAW 格式的 FAT32 镜像。
- 遍历文件系统，读取文件内容。
- 支持 NT 对短文件名的扩展。

## 计划开发的功能

- 增加写入文件系统的功能。
- 重构代码，统一接口。

## 已知的 Bug

## Demo test:

1. `sudo apt install build-essential g++ mtools`
2. 先运行脚本 `sandbox/init.sh` 创建镜像 `raw.img`，再运行脚本 `sandbox/sync.sh` 将 `sandbox/data/` 下的文件写入到镜像中。
3. `make run`。

不出意外的话，程序会遍历 `raw.img` 包含的文件系统中的所有文件。

## Miscs

本项目仍在开发早期，如果发现了什么 Bug 欢迎发 issue！