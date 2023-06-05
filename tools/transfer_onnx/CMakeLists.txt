cmake_minimum_required(VERSION 3.10)

project(tensorrt_inference)

find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(/usr/src/tensorrt/samples/common)
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(/usr/include/aarch64-linux-gnu)

# add_executable(tensorrt_inference sampleOnnxMNIST.cpp /usr/src/tensorrt/samples/common/logger.cpp)
add_executable(tensorrt_inference infer.cpp /usr/src/tensorrt/samples/common/logger.cpp)

target_link_libraries(tensorrt_inference /usr/lib/aarch64-linux-gnu)
target_link_libraries(tensorrt_inference ${CUDA_LIBRARIES})
target_link_libraries(tensorrt_inference ${OpenCV_LIBS})
target_link_libraries(tensorrt_inference nvinfer)
target_link_libraries(tensorrt_inference nvparsers)
target_link_libraries(tensorrt_inference nvonnxparser)


add_executable(trans trans.cpp /usr/src/tensorrt/samples/common/logger.cpp)
target_link_libraries(trans /usr/lib/aarch64-linux-gnu)
target_link_libraries(trans ${CUDA_LIBRARIES})
target_link_libraries(trans ${OpenCV_LIBS})
target_link_libraries(trans nvinfer)
target_link_libraries(trans nvparsers)
target_link_libraries(trans nvonnxparser)
