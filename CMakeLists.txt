cmake_minimum_required(VERSION 3.16)
project(Aemeath VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动调用 moc, uic, rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt6 REQUIRED COMPONENTS Multimedia MultimediaWidgets)

# 查找 OpenCV
find_package(OpenCV REQUIRED)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/desktop_pet.cpp include/desktop_pet.h
    src/settings_menu.cpp include/settings_menu.h
    src/clock_menu.cpp include/clock_menu.h
    src/ai_chat_dialog.cpp include/ai_chat_dialog.h
    include/native_web_view.h
    include/wke.h
    include/AVLTree.h include/AVLTree.inl
    assets/resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接 Qt6 库
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
)

# 复制 Miniblink 运行时到可执行文件目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/node.dll"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE FALSE
)