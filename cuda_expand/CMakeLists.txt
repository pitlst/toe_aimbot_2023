cmake_minimum_required(VERSION 3.10)
project(cuda_expand)

find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})

# 添加可执行文件
cuda_add_executable(${PROJECT_NAME} cuda_expand.cuh preprocess.cu postprocess.cu)

# 链接CUDA库
target_link_libraries(${PROJECT_NAME} ${CUDA_LIBRARIES} ${OpenCV_LIBS})