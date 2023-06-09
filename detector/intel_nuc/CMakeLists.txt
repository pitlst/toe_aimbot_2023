cmake_minimum_required(VERSION 3.10)

project(openvino_test)
find_package(OpenCV REQUIRED)

#add_definitions(-DUSE_NVIDIA)

include_directories(./)
include_directories(../)
include_directories(../../)
include_directories(/home/cheakf/Downloads/l_openvino_toolkit_ubuntu20_2022.3.0.9052.9752fafe8eb_x86_64/runtime/include/ie)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(ov_test ov_main.cpp  ../detector.cpp ../parse_config.cpp)
target_link_libraries(ov_test ${OpenCV_LIBS} jsoncpp)


