#--------------------------------------------------------------------------------------#
#   项目总体设置
#--------------------------------------------------------------------------------------#
cmake_minimum_required(VERSION 3.12)
# 设置项目名称
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
# 设置编译优化的等级
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
# 判断项目平台
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    message(STATUS ${CMAKE_HOST_SYSTEM_NAME} " 检测到为linux平台")
else()
	message(FATAL_ERROR ${CMAKE_HOST_SYSTEM_NAME} " 检测到不是linux平台,该平台目前未支持") 
endif()
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS ${CMAKE_HOST_SYSTEM_PROCESSOR} " 检测到为ARM平台,启用tensorRT" )
    set(TENSORRT_START 1)
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    message(STATUS ${CMAKE_HOST_SYSTEM_PROCESSOR} " 检测到为x86平台,启用openvino" )
    set(OPENVINO_START 1)
else()
	message(FATAL_ERROR  ${CMAKE_HOST_SYSTEM_PROCESSOR} " 检测到不不支持的架构")
endif()
enable_testing()

#--------------------------------------------------------------------------------------#
#   项目依赖设置
#--------------------------------------------------------------------------------------#
# 找到海康动态库，保存到变量里
FILE (GLOB HIK_LIBS "/opt/MVS/lib/64/*.so")
# 找到海康头文件，保存到变量里
FILE (GLOB HIK_INCLUDE "/opt/MVS/include/*.h")
include_directories(/opt/MVS/include)
# 找到opencv
find_package(OpenCV REQUIRED)
# 设置头文件查找路径
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    asio_1.28
    general
    camera
    serial
    detect
)
# 找到openvino
if(OPENVINO_START)
    find_package(OpenVINO REQUIRED)
    if(NOT OPENVINO_START)
        message(FATAL_ERROR "Cannot find openvino library.")
    endif()
    message(STATUS "已找到openvino,路径" ${OpenVINO_DIR})
    include_directories(${OpenVINO_INCLUDE_DIRS})
    include_directories(detect/intel_nuc_11)
# 找到TensorRT
elseif(TENSORRT_START)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES include)
    message(STATUS "Found TensorRT headers at ${TENSORRT_INCLUDE_DIR}")
    find_library(TENSORRT_LIBRARY_INFER nvinfer HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
    find_library(TENSORRT_LIBRARY_INFER_PLUGIN nvinfer_plugin HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
    find_library(TENSORRT_LIBRARY_INFER_PARSER nvparsers HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR} PATH_SUFFIXES lib lib64 lib/x64)
    set(TENSORRT_LIBRARY ${TENSORRT_LIBRARY_INFER} ${TENSORRT_LIBRARY_INFER_PLUGIN} ${TENSORRT_LIBRARY_INFER_PARSER})
    MESSAGE(STATUS "Found TensorRT libs at ${TENSORRT_LIBRARY}")
    find_package_handle_standard_args(TENSORRT DEFAULT_MSG TENSORRT_INCLUDE_DIR TENSORRT_LIBRARY)
    if(NOT TENSORRT_FOUND)
        message(FATAL_ERROR "Cannot find TensorRT library.")
    endif()
    message(STATUS "已找到TensorRT" ${TENSORRT_ROOT})
    include_directories(detect/nvidia_orin_nx)
    include_directories(${TENSORRT_INCLUDE_DIR})
    add_compile_definitions(USE_NVIDIA)
    set(openvino::runtime "")
endif()
# 把项目目录定义成宏传递给程序
add_compile_definitions(PROJECT_PATH=\"${PROJECT_SOURCE_DIR}\")

#--------------------------------------------------------------------------------------#
#   项目编译设置
#--------------------------------------------------------------------------------------#
add_executable(${PROJECT_NAME} 
                ${HIK_INCLUDE}
                debug.hpp
                general/toe_json.hpp
                general/toe_structs.hpp
                camera/grab_img.hpp
                camera/grab_img.cpp
                serial/toe_serial.hpp
                serial/toe_serial.cpp 
                detect/detect.hpp
                detect/detect.cpp
                detect/intel_nuc_11/ov_detect.hpp
                detect/intel_nuc_11/ov_detect.cpp
                detect/nvidia_orin_nx/orin_detect.hpp
                detect/nvidia_orin_nx/orin_detect.hpp
                main.cpp 
                )
target_link_libraries(${PROJECT_NAME} PRIVATE pthread ${OpenCV_LIBS} ${HIK_LIBS} ${TENSORRT_LIBRARY} openvino::runtime)

# add_executable(serial_test
#                 debug.hpp
#                 general/toe_json.hpp
#                 general/toe_structs.hpp
#                 serial/toe_serial.hpp
#                 serial/toe_serial.cpp 
#                 test/serial_test.cpp
#                 )

add_executable(grab_test
                debug.hpp
                ${HIK_INCLUDE}
                general/toe_json.hpp
                general/toe_structs.hpp
                camera/grab_img.hpp
                camera/grab_img.cpp 
                test/grab_test.cpp
                )
target_link_libraries(grab_test PRIVATE ${OpenCV_LIBS} ${HIK_LIBS})


# add_executable(buffer_test
#                 test/buffer_test.cpp
#                 )

add_executable(ov_test
                debug.hpp
                general/toe_json.hpp
                general/toe_structs.hpp
                detect/detect.hpp
                detect/detect.cpp
                detect/intel_nuc_11/ov_detect.hpp
                detect/intel_nuc_11/ov_detect.cpp
                test/ov_test.cpp
                )
target_link_libraries(${PROJECT_NAME} PRIVATE pthread ${OpenCV_LIBS} openvino::runtime)

# add_executable(orin_test
#                 debug.hpp
#                 general/toe_json.hpp
#                 general/toe_structs.hpp
#                 detect/detect.hpp
#                 detect/detect.cpp
#                 detect/intel_nuc_11/ov_detect.hpp
#                 detect/intel_nuc_11/ov_detect.cpp
#                 test/orin_test.cpp
#                 )
# target_link_libraries(${PROJECT_NAME} PRIVATE pthread ${OpenCV_LIBS} ${TENSORRT_LIBRARY})


#--------------------------------------------------------------------------------------#
#   项目测试设置
#--------------------------------------------------------------------------------------#
# add_test(serial_test serial_test)
# add_test(grab_test grab_test)
# add_test(buffer_test buffer_test)
# add_test(ov_test ov_test)
# add_test(orin_test orin_test)