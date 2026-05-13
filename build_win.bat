@echo off
:: 创建构建目录（如果不存在）
if not exist "build" mkdir build
cd build

:: 生成 MinGW Makefiles，并指定 Qt6 的 MinGW 版本路径
:: 注意：路径要指向 Qt 6.9.2 的 mingw_64 下的 lib\cmake\Qt6
cmake .. -G "MinGW Makefiles" -DQt6_DIR="D:\qt\6.9.2\mingw_64\lib\cmake\Qt6"

:: 编译项目（默认是 Debug 模式，若要 Release 需加 --config Release）
:: -j8 表示用 8 线程编译，可根据 CPU 核心数调整
cmake --build . -j8

:: 提示结果（MinGW 编译的可执行文件在 build 根目录，非 Release 子目录）
echo 构建完成！可执行文件在：build\MarkdownEditor.exe
cd ..
pause