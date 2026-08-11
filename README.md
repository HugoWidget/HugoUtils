# HugoUtils

## 项目介绍

HugoUtils 是 Hugo 系列工具的核心库，采用 C++ 编写，提供了一套完整的 Windows 系统功能封装，包括冰点还原管理、希沃管家控制、虚拟磁盘挂载、密码恢复等。  
为便于跨语言集成，本库提供纯 C 语言接口绑定，并编译为 Windows 动态链接库（`HugoUtils.dll`）。  
使用动态链接方式调用本库，无需将您的应用程序开源，但若您修改本库本身，则需遵循 LGPLv3 许可证。

## 功能模块

| 功能模块 | 说明 |
| :--- | :--- |
| 冰点状态管理 | 查询/设置保护状态、尝试保护 |
| 冰点驱动通信 | 直接 IOCTL 与驱动交互 |
| 冰点配置文件读写 | 读写 `ProtectInfo` 配置文件 |
| 虚拟磁盘挂载 | 挂载/卸载 VHD 等虚拟磁盘 |
| HugoLock 进程间共享标志 | 跨进程同步标志（共享内存） |
| 希沃信息查询 | 获取版本、路径、机器码等 |
| ASCII 艺术字 | 打印/获取带艺术字的状态文本 |
| GPL 许可展示 | 显示保修和许可证信息 |
| 密码破解 | 破解希沃管家加密密码 |
| HTTP 下载器（DLL中未导出） | 支持断点续传的下载工具 |

## 环境要求

- **操作系统**：Windows 10/11
- **开发工具**：Visual Studio 2022

## 构建

1. 克隆仓库并更新子模块。
2. 打开 `HugoUtils.sln`，选择 `Release x64/x86` 配置，选择`配置类型`（`.dll`/`.lib`）

   如果选择`.dll`，则需要添加`HUGOUTILS_EXPORTS`宏
   
   如果选择`.lib`，则需要添加`HUGOUTILS_NO_EXPORTS`宏（默认选项）
3. 生成解决方案，输出 `HugoUtils.dll` 或对应的导入库 `HugoUtils.lib`。

## 使用示例（C 语言绑定）

```c
#include "HugoUtilsC.h"

HugoFreezeApi* api = Hugo_FreezeApi_Create();
if (api) {
    Hugo_FreezeApi_Init(api);
    wchar_t msg[256];
    HugoResult res = Hugo_FreezeApi_GetFreezeState(api, msg, sizeof(msg)/sizeof(wchar_t));
    // ...
    Hugo_FreezeApi_Destroy(api);
}
```

## 项目依赖

[WinUtils](https://github.com/howdy213/WinUtils)(Windows微功能组件库，已内置)

[cpp-httplib](https://github.com/yhirose/cpp-httplib)(HTTP/HTTPS网络库，依赖OpenSSL，已内置)

[hash-library](https://github.com/stbrumme/hash-library)(哈希库，已内置)

## 免责声明

本项目仅供研究和教育目的使用。用户不得将其用于违反当地法律法规、侵犯他人著作权或违反软件 EULA 的用途。任何非法使用所带来的后果由使用者自行承担，开发者不承担任何连带责任。

## 许可证

本项目采用 LGPLv3 许可证，详情参见 [LICENSE](LICENSE) 与 [LICENSE.LESSER](LICENSE.LESSER) 文件。

WinUtils:  [MIT 许可证](licenses/LICENSE-WinUtils)

hash-library: [zlib 许可证](licenses/LICENSE-hash-library)

swhelper：[MIT 许可证](licenses/LICENSE-swhelper)

cpp-httplib: [MIT 许可证](licenses/LICENSE-cpp-httplib)

mINI: [MIT 许可证](licenses/LICENSE-mINI)

WinReg: [MIT 许可证](licenses/LICENSE-WinReg)

libsharedmemory：[MIT 许可证](licenses/LICENSE-libsharedmemory)
