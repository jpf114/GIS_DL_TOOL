# 安装包示例

本目录用于验证安装后的 `gis_ai` 包是否可以被外部 CMake 工程正常消费。

示例用法：

```bash
cmake -S examples/installed_package -B build/installed-package-demo -DCMAKE_PREFIX_PATH=/path/to/install/prefix
cmake --build build/installed-package-demo
```

该示例默认通过：

```cmake
find_package(gis_ai CONFIG REQUIRED)
target_link_libraries(<target> PRIVATE gis_ai::gis_ai)
```

来验证安装导出的目标是否可用。
