cmake_minimum_required(VERSION 3.10)

project(openvino_test)
find_package(OpenCV REQUIRED)

include_directories(./)
include_directories(../)
include_directories(../../)
include_directories(${OpenCV_INCLUDE_DIRS})

target_link_libraries(ov_test ${OpenCV_LIBS} jsoncpp)

add_executable(ov_test ov_main.cpp ov_detect.cpp detector.cpp parse_config.cpp)
