#ifdef NX

#include "detector.hpp"

/**
 * @brief convert cv::Mat to Data (aka std::vector<float>)
 * 
 * @param img source image
 * @param blob destination
 */
void Detector::copyBlob(Data &from, Data& to) {
    memcpy(to.data(), from.data(), sizeof(float) * from.size());
}

/**
 * @brief make the dim information easy to print
 * 
 * @param dims 
 * @return std::string 
 */
std::string vecsize_to_string(nvinfer1::Dims dims) {
    std::string size_str;
    for (int i = 0; i < dims.nbDims; ++i) {
        size_str += std::to_string(dims.d[i]) + (i < dims.nbDims - 1 ? "x" : "");
    }
    return size_str;
}

#endif // NX