cmake_minimum_required(VERSION 3.19)
project(MarkdownEditor LANGUAGES CXX)

# 1. 强制 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. 启用 Qt 自动工具（MOC/UIC/RCC）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. 处理编码和 C++17 特性（MSVC 特有）
if(MSVC)
    add_compile_options(/utf-8)       # 强制 UTF-8 编译
    add_compile_options(/Zc:__cplusplus)  # 正确设置 __cplusplus
endif()

# 4. 查找 Qt6 组件
find_package(Qt6  REQUIRED COMPONENTS
    Core
    Widgets
    PrintSupport
    Network
)

# 5. 设置构建类型（Debug/Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the build type (Debug or Release)" FORCE)
endif()

# 6. 配置编译选项（区分 MSVC 和 GCC/Clang）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/Zi /Od)  # MSVC Debug：调试信息 + 禁用优化
    else()
        add_compile_options(-g -O0)   # GCC/Clang Debug
    endif()
else()
    if(MSVC)
        add_compile_options(/O2 /DNDEBUG)  # MSVC Release：优化 + 禁用调试
    else()
        add_compile_options(-O2 -DNDEBUG)  # GCC/Clang Release
    endif()
endif()

# 7. 资源文件（若有）
set(RESOURCE_FILES
    # resources.qrc
)
if(RESOURCE_FILES)
    qt_add_resources(RESOURCE_SOURCES ${RESOURCE_FILES})
endif()

# 8. 源文件和头文件
set(SOURCES
    main.cpp
    mainwindow.cpp
    markdownparser.cpp
    markdownhighlighter.cpp
    editorhighlighter.cpp
    fileencodingdetector.cpp
    appsettings.cpp
    diffrenderer.cpp
    virtualizededitor.cpp
    chatbubble.cpp          # ← 新增
    chatmodel.cpp           # ← 新增
    chatpanel.cpp           # ← 新增
    deepseekclient.cpp      # ← 新增
    contextbuilder.cpp      # ← 新增
    ${RESOURCE_SOURCES}
)
set(HEADERS
    mainwindow.h
    markdownparser.h
    markdownhighlighter.h
    editorhighlighter.h
    fileencodingdetector.h
    appsettings.h
    diffrenderer.h
    virtualizededitor.h
    chatbubble.h            # ← 新增
    chatmodel.h             # ← 新增
    aiclient.h              # ← 新增
    chatpanel.h             # ← 新增
    deepseekclient.h        # ← 新增
    contextbuilder.h        # ← 新增
)
set(UIS
    mainwindow.ui
)

# 9. 创建可执行文件
qt_add_executable(MarkdownEditor
    WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${UIS}

)

# 10. 链接 Qt 库
target_link_libraries(MarkdownEditor
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::PrintSupport
        Qt6::Network
)

# 11. 安装和部署（可选）
include(GNUInstallDirs)
install(TARGETS MarkdownEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
qt_generate_deploy_app_script(
    TARGET MarkdownEditor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# 12. 编译定义（可选，若不需要 QT_NO_KEYWORDS 可删除）
target_compile_definitions(MarkdownEditor
    PRIVATE
        APP_VERSION="1.0.0"
        # QT_NO_KEYWORDS  # 若不需要禁用 Qt 关键字，注释此行
)
