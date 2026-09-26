# HugoUtils

> [!NOTE]
> 建议阅读 [HugoWidget 简介](https://github.com/HugoWidget/HugoWidget) 以了解开发情况。

## 项目介绍

HugoUtils 是 Hugo 系列工具的核心库，采用 C++ 编写，提供了一套完整的 Windows 系统功能封装，包括冰点还原管理、希沃管家控制、虚拟磁盘挂载、密码恢复等。

为便于跨语言集成，本库提供纯 C 语言接口绑定，并编译为 Windows 动态链接库（`HugoUtils.dll`）。

## 功能模块

| 功能模块                    | 说明                        |
| :-------------------------- | :-------------------------- |
| 冰点状态管理                | 查询/设置保护状态、尝试保护 |
| 冰点驱动通信                | 直接 IOCTL 与驱动交互       |
| 冰点配置文件读写            | 读写 `ProtectInfo` 配置文件 |
| 虚拟磁盘挂载                | 挂载/卸载 VHD 等虚拟磁盘    |
| HugoLock 进程间共享标志     | 跨进程同步标志（共享内存）  |
| 希沃信息查询                | 获取版本、路径、机器码等    |
| ASCII 艺术字                | 打印/获取带艺术字的状态文本 |
| GPL 许可展示                | 显示许可证信息              |
| 密码破解                    | 破解希沃管家加密密码        |
| HTTP 下载器（DLL 中未导出） | 支持断点续传的下载工具      |

## 环境要求

- **操作系统**：Windows 10/11
- **开发工具**：Visual Studio 2022

## 构建

1. 克隆仓库并更新子模块。
2. 打开 `HugoUtils.slnx`，选择 `Release x64/x86` 配置，选择配置类型（`.dll` / `.lib`）。
   - 如果选择 `.dll`，需要添加 `HUGOUTILS_EXPORTS` 宏。
   - 如果选择 `.lib`，需要添加 `HUGOUTILS_NO_EXPORTS` 宏（默认选项）。
3. 生成解决方案，输出 `HugoUtils.dll` 或对应的导入库 `HugoUtils.lib`。

## 使用示例（C 语言绑定）

```c
#include "HugoUtilsC.h"

HugoFreezeApi* api = Hugo_FreezeApi_Create();
if (api) {
    Hugo_FreezeApi_Init(api);

    wchar_t msg[256];
    HugoResult res = Hugo_FreezeApi_GetFreezeState(
        api,
        msg,
        sizeof(msg) / sizeof(wchar_t)
    );

    // ...

    Hugo_FreezeApi_Destroy(api);
}
```

## 使用说明

我们支持开源生态，建议开发者开源项目并使用本库的静态链接，以获得更方便的调用体验。

### 动态链接

HugoUtils 以 LGPLv3 许可证发布。若要在闭源项目中使用，建议始终以**动态链接**方式使用 `HugoUtils.dll`，并遵守 LGPLv3 的相关要求。若你修改了库本身，则需要按 LGPLv3 发布修改后的库，并继续允许最终用户替换库版本。

#### 方式一：使用 C 接口

C 接口具有更好的跨语言、跨编译器兼容性，推荐优先使用。

1. 从 Release 中获取，或自行构建得到以下文件：
   - `HugoUtilsC.h`
   - `HugoUtils.dll`
   - `HugoUtils.lib`（如使用隐式链接）
2. 将 `HugoUtilsC.h` 所在目录加入项目的头文件搜索路径。
3. 如使用隐式链接，将 `HugoUtils.lib` 加入项目的链接器输入。
4. 将 `HugoUtils.dll` 放到应用程序输出目录，或放到系统 `PATH` 可搜索到的目录中。
5. 确保目标程序与 `HugoUtils.dll` 的架构一致：
   - x86 程序使用 x86 版 DLL
   - x64 程序使用 x64 版 DLL
6. 调用时遵循“创建 → 初始化 → 调用 → 销毁”的生命周期。

#### 方式二：使用 C++ 接口

C++ 接口默认未直接导出，主要出于 ABI 兼容性考虑。若确实需要 C++ 接口，可自行 fork 仓库并按需导出。

1. fork 仓库。
2. 根据需要添加 C++ 接口的导出声明。
3. 构建 DLL 时定义 `HUGOUTILS_EXPORTS` 宏。
4. 构建静态库或不导出时定义 `HUGOUTILS_NO_EXPORTS` 宏（默认选项）。
5. 按 LGPLv3 要求发布修改后的库。

使用 C++ 接口时，调用方与库应尽量保持一致：

- 相同编译器版本
- 相同运行库配置
- 相同架构：x86 / x64
- 相同构建配置：Debug / Release

否则可能出现 ABI 不兼容、链接失败或运行时崩溃。

## 项目依赖

- [WinUtils](https://github.com/howdy213/WinUtils)：Windows 微功能组件库，已内置。
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)：HTTP/HTTPS 网络库，依赖 OpenSSL，已内置。
- [hash-library](https://github.com/stbrumme/hash-library)：哈希库，已内置。

## 许可证

本项目采用 LGPLv3 许可证，详情参见 [LICENSE](LICENSE) 与 [LICENSE.LESSER](LICENSE.LESSER) 文件。

第三方许可证：

- WinUtils：[MIT 许可证](licenses/LICENSE-WinUtils)
- hash-library：[zlib 许可证](licenses/LICENSE-hash-library)
- swhelper：[MIT 许可证](licenses/LICENSE-swhelper)
- cpp-httplib：[MIT 许可证](licenses/LICENSE-cpp-httplib)
- mINI：[MIT 许可证](licenses/LICENSE-mINI)
- WinReg：[MIT 许可证](licenses/LICENSE-WinReg)
- libsharedmemory：[MIT 许可证](licenses/LICENSE-libsharedmemory)
- OpenSSL：[Apache-2.0 license](licenses/LICENSE-OpenSSL)

## 免责声明

本项目仅供研究和教育目的使用。用户不得将其用于违反当地法律法规、侵犯他人著作权或违反软件 EULA 的用途。任何非法使用所带来的后果由使用者自行承担，开发者不承担任何连带责任。
