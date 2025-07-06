# U-Boot System Update Manager (USUM)

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## 项目简介

USUM 是一个基于 U-Boot 的嵌入式系统更新管理平台，支持内核（Kernel）、根文件系统（RootFS）和应用程序（App）的分模块动态升级。  
项目设计以工业级稳定性和易用性为核心，支持通过 U 盘或 OTA 网络自动检测升级包，并提供交互式升级菜单。  

USUM 能在设备启动前完成升级操作，确保系统在异常或崩溃时依然可恢复，极大提升设备的维护效率和运行稳定性。  
同时项目注重平台无关性，具备良好的可移植性，适配多种嵌入式硬件平台（如 RK3568、Allwinner 等）。

---

## 主要功能

- 支持通过 U-Boot 菜单交互式选择更新内容（Kernel / RootFS / App）
- 支持 USB U 盘自动扫描升级包（支持 FAT 文件系统）
- 支持基于 DHCP+TFTP / HTTP 的 OTA 网络升级
- 镜像文件校验与版本管理，防止升级失败导致设备异常
- 支持升级失败自动回滚和恢复
- 配置灵活，易于集成到不同硬件平台和启动流程
- 开放源码，方便二次开发和定制
