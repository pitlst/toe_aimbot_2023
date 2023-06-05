#ifndef NX

#include "detector.hpp"

/**
 * @brief write the image(cv::Mat) to the float array (wrapped by InferenceEngine)
 * 
 * @param img source image
 * @param blob destination
 */
void Detector::copyBlob(Data &blob, InferenceEngine::Blob::Ptr &ieBlob) {
    InferenceEngine::MemoryBlob::Ptr mblob =
        InferenceEngine::as<InferenceEngine::MemoryBlob>(ieBlob);
    if (!mblob) {
        THROW_IE_EXCEPTION
            << "We expect blob to be inherited from MemoryBlob in matU8ToBlob, "
            << "but by fact we were not able to cast inputBlob to MemoryBlob";
    }
    // locked memory holder should be alive all time while access to its buffer
    // happens
    auto mblobHolder = mblob->wmap();

    float *ie_blob_data = mblobHolder.as<float *>();

    memcpy(ie_blob_data, blob.data(), sizeof(float) * blob.size());
}

/**
 * @brief make the dim information easy to print
 * 
 * @param vec 
 * @return std::string 
 */
std::string vecsize_to_string(std::vector<size_t> vec) {
    std::string size_str;
    for (int i = 0; i < vec.size(); ++i) {
        size_str += std::to_string(vec[i]) + (i < vec.size() - 1 ? "x" : "");
    }
    return size_str;
}

#endif // not NX