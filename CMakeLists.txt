cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(PROJECT_NAME toe_aimbot_2023)
# 在这里直接指定项目为c++项目，节约查找tool chain的时间
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)
# 设置c++标准等级和编译后输出路径
set(CMAKE_CXX_STANDARD 11)
# 导出执行命令到json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 强制化c++标准，当标准不符合要求时强制终止编译而不是采用更旧的标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 找到所有第三方动态库，保存到变量里
FILE (GLOB 3rd_LIBS "/opt/MVS/lib/64/*.so")
# 找到所有第三方头文件，保存到变量里
FILE (GLOB 3rd_INCLUDE "/opt/MVS/include/*.h")

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS filesystem regex REQUIRED)

# 打印相关信息
message(STATUS "项目名称 ${PROJECT_NAME}")
message(STATUS "目标平台是 ${CMAKE_SYSTEM}")
message(STATUS "编译等级是 ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenCV版本是 ${OpenCV_VERSION}")

include_directories(
  ${OpenCV_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  include
  3rd_part/include
)

add_executable(${PROJECT_NAME} 
                src/main.cpp 
                src/get_frame.cpp 
                src/serial.cpp 
                include/debug.h 
                include/get_frame.h
                include/logger.h 
                include/serial.h 
                include/toe_json.h 
                ${3rd_INCLUDE})

target_link_libraries(${PROJECT_NAME} PRIVATE pthread ${OpenCV_LIBS} ${Boost_LIBRARIES} ${DEPENDENCIES} ${3rd_LIBS})
