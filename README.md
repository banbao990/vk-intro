# vulkan-intro
+ 《计算机图形学》本科生课程 vulkan 代码示例

## 配置
+ CMake 工程
+ 本项目仅依赖于 vulkan-sdk
    + [vulkan-sdk](https://vulkan.lunarg.com/)
+ 测试的版本是 1.3.236.0
+ 已经测试成功可以运行的环境
    + windows
        + visual studio 2022
        + vscode + 课程给出的 mingw
+ 如果本地没有 SDL 库，需要将 CMakeLists.txt 中的第 8 行修改如下
```txt
set(USE_THIRD_PARTY_SDL ON)
```

+ 更多问题请查看课程给出的[腾讯文档](https://docs.qq.com/doc/DUnRZTmV3ek1wb1ZY)

## 参考项目
+ https://github.com/vblanco20-1/vulkan-guide/tree/all-chapters
+ https://github.com/SaschaWillems/Vulkan
+ https://github.com/iOrange/rtxON
