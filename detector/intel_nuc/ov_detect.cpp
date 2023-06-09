/*
* ov_detector.cpp
* Created on: 20230606
* Author: sumang
* Description: armor detector of openvino
*/

#include <unistd.h>
#include "ov_detect.h"

using namespace cv;
using namespace std;

OvO_Detector::OvO_Detector()
{
    int NUM_CLASSES = 8;
    int NUM_SIZES = 2;
    int NUM_COLORS = 4;
    network_ = ie_.ReadNetwork(param_.xml_file_path, param_.bin_file_path);
    input_name_ = network_.getInputsInfo().begin()->first;
    for (auto iter : network_.getOutputsInfo())
    {
        auto dims = iter.second->getDims();
        int stride_h = param_.h / dims[2];
        int stride_w = param_.h / dims[3];
        assert(stride_h == stride_w && "Invalid stride!");
        output_layers_.push_back((s_OutLayer){
            .idx = (int)output_names_.size(),
            .stride = stride_h,
            .num_anchor = (int)dims[1],
            .num_out = (int)dims[4]});
        output_names_.push_back(iter.first);
        assert(dims[4] == 5 + 10 + NUM_CLASSES + NUM_COLORS + NUM_SIZES && "Output dimension wrong!");
        iter.second->setPrecision(InferenceEngine::Precision::FP32);
    }
    executable_network_ = ie_.LoadNetwork(network_, "GPU");
    infer_request_ = executable_network_.CreateInferRequest();
    cout << "network_init_done. " << endl;
}

void OvO_Detector::copyBlob(vector<float> &blob, InferenceEngine::Blob::Ptr &ieBlob) {
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

void OvO_Detector::preprocess()
{
    int img_h = input_img_.rows;
    int img_w = input_img_.cols;

    float *blob_data = blob.data();

    size_t i = 0;
    for (size_t row = 0; row < img_h; ++row)
    {
        uchar *uc_pixel = input_img_.data + row * input_img_.step;
        for (size_t col = 0; col < img_w; ++col)
        {
            // 三通道
            blob_data[i] = (float)uc_pixel[2] / 255.0;
            blob_data[i + img_h * img_w] = (float)uc_pixel[1] / 255.0;
            blob_data[i + 2 * img_h * img_w] = (float)uc_pixel[0] / 255.0;
            uc_pixel += 3;
            ++i;
        }
    }
}
void OvO_Detector::inference()
{
    InferenceEngine::Blob::Ptr ieBlob = infer_request_.GetBlob(input_name_);
    copyBlob(blob, ieBlob);
    auto infer_start = std::chrono::steady_clock::now();
    infer_request_.Infer();
}
void OvO_Detector::postprocess()
{
    auto decode_start = std::chrono::steady_clock::now();
    s_detections objects;
    for (auto layer : output_layers_)
    {
        const InferenceEngine::Blob::Ptr output_blob =
            infer_request_.GetBlob(output_names_[layer.idx]);
        InferenceEngine::MemoryBlob::CPtr moutput =
            InferenceEngine::as<InferenceEngine::MemoryBlob>(output_blob);
        auto moutputHolder = moutput->rmap();
        const float *net_pred =
            moutputHolder.as<const InferenceEngine::PrecisionTrait<
                InferenceEngine::Precision::FP32>::value_type *>();

        // decode_outputs(net_pred, objects, layer, param_.NCHW.w, param_.NCHW.h);
    }
}
bool OvO_Detector::detect()
{
    while (input_imgs_.empty()) usleep(10);
    img_mutex_.lock();
    input_img_ = input_imgs_.back();
    img_mutex_.unlock();

    preprocess();
    inference();


}

/*
static void decode_outputs(const float *prob, s_detections &objects,
                              ml::OutLayer layer_info, const int img_w,
                              const int img_h)
{
    float scale = std::min(param_.NCHW.w / (img_w * 1.0), param_.NCHW.h / (img_h * 1.0));

    std::vector<float> *l_anchor = nullptr;
    switch (layer_info.stride)
    {
        case 4:
            l_anchor = anchors;
            break;
        case 8:
            l_anchor = anchors+1;
            break;
        case 16:
            l_anchor = anchors+2;
            break;
        case 32:
            l_anchor = anchors+3;
            break;
        default:
            assert(false && "Unknown layer stride!");
            break;
    }

    int out_h = param_.NCHW.h / layer_info.stride;
    int out_w = param_.NCHW.w / layer_info.stride;
    float pred_data[layer_info.num_out];
    for (int na = 0; na < 3; ++na)
    {
        for (int h_id = 0; h_id < out_h; ++h_id)
        {
            for (int w_id = 0; w_id < out_w; ++w_id)
            {
                int data_idx = (na * out_h * out_w + h_id * out_w + w_id) *
                            layer_info.num_out;
                float obj_conf = sigmoid(prob[data_idx + 4]);
                if (obj_conf > BBOX_CONF_THRESH)
                {
                    // std::cout << obj_conf << std::endl;
                    sigmoid(prob + data_idx, pred_data, 5);
                    sigmoid(prob + data_idx + 15, pred_data + 15,
                            NUM_CLASSES + NUM_COLORS + 2);
                    memcpy(pred_data + 5, prob + data_idx + 5,
                        sizeof(float) * 10);
                    int col_id = std::max_element(pred_data + 15 + NUM_CLASSES,
                                                pred_data + 15 + NUM_CLASSES +
                                                    NUM_COLORS) -
                                (pred_data + 15 + NUM_CLASSES);
                    int cls_id =
                        std::max_element(pred_data + 15,
                                        pred_data + 15 + NUM_CLASSES) -
                        (pred_data + 15);
                    
                    int t_size = std::max_element(pred_data + 15 + NUM_CLASSES + NUM_COLORS,
                            pred_data + 15 + NUM_CLASSES + NUM_COLORS + 2) -
                    (pred_data + 15 + NUM_CLASSES + NUM_COLORS);

                    double final_conf =
                        obj_conf * pow(pred_data[15 + cls_id] *
                                        pred_data[15 + NUM_CLASSES + col_id] *
                                        pred_data[15 + NUM_CLASSES + NUM_COLORS + t_size],
                                        1/3.);
                    if (final_conf > BBOX_CONF_THRESH)
                    {
                        // std::cout << final_conf << " " << col_id << " "
                        //           << cls_id << std::endl;
                        armor::Armor now;
                        float x = (pred_data[0] * 2.0 - 0.5 + w_id) *
                                layer_info.stride;
                        float y = (pred_data[1] * 2.0 - 0.5 + h_id) *
                                layer_info.stride;
                        float w =
                            pow(pred_data[2] * 2, 2) * l_anchor->at(na * 2);
                        float h =
                            pow(pred_data[3] * 2, 2) * l_anchor->at(na * 2 + 1);

                        for (int p = 0; p < 5; ++p)
                        {
                            float px =
                                (pred_data[5 + p * 2] * l_anchor->at(na * 2) +
                                w_id * layer_info.stride) /
                                scale;
                            float py = (pred_data[5 + p * 2 + 1] *
                                            l_anchor->at(na * 2 + 1) +
                                        h_id * layer_info.stride) /
                                    scale;
                            px = std::max(std::min(px, (float)(img_w)), 0.f);
                            py = std::max(std::min(py, (float)(img_h)), 0.f);
                            now.pts[p] = cv::Point2f(px, py);
                            // std::cout << px << " " << py  << " ";
                        }
                        // std::cout << std::endl;

                        float x0 = (x - w * 0.5) / scale;
                        float y0 = (y - h * 0.5) / scale;
                        float x1 = (x + w * 0.5) / scale;
                        float y1 = (y + h * 0.5) / scale;

                        x0 = std::max(std::min(x0, (float)(img_w)), 0.f);
                        y0 = std::max(std::min(y0, (float)(img_h)), 0.f);
                        x1 = std::max(std::min(x1, (float)(img_w)), 0.f);
                        y1 = std::max(std::min(y1, (float)(img_h)), 0.f);

                        now.rect = cv::Rect(x0, y0, x1 - x0, y1 - y0);
                        now.conf = final_conf;
                        now.color = col_id;
                        now.type = cls_id;
                        now.t_size = t_size;
                        objects.push_back(now);
                    }
                }
            }
        }
    }

    int no = layer_info.num_out;
    float pred_data[no];
    // [x, y, w, h, conf, (x,y)*5, hot(class), hot(color)]
    for (int idx = 0; idx < 8400; ++idx)
    {
        float rough_conf = *std::max_element(&prob[idx * no + NUM_KPTS],
                                        &prob[idx * no + no]);
        
        if (rough_conf > BBOX_CONF_THRESH)
        {
            std::memcpy(pred_data, &prob[idx * no], no*sizeof(float));
            int col_id = std::max_element(pred_data + NUM_KPTS + NUM_CLASSES + 2,
                                        pred_data + NUM_KPTS + NUM_CLASSES + 2 + 
                                                NUM_COLORS) -
                            (pred_data + NUM_KPTS + NUM_CLASSES + 2);
            int cls_id =
                std::max_element(pred_data + NUM_KPTS,
                                    pred_data + NUM_KPTS + NUM_CLASSES ) -
                            (pred_data + NUM_KPTS);

            int t_size = std::max_element(pred_data + NUM_KPTS + NUM_CLASSES,
                                    pred_data + NUM_KPTS + NUM_CLASSES + 2) -
                            (pred_data + NUM_KPTS + NUM_CLASSES);

            double final_conf = std::min({pred_data[NUM_KPTS + NUM_CLASSES + 2 + col_id],  
                                        pred_data[NUM_KPTS + cls_id]});
            if (final_conf > BBOX_CONF_THRESH)
            {
                // std::cout << final_conf << " " << col_id << " "
                //           << cls_id << std::endl;
                armor::Armor now;

                for (int p = 0; p < (NUM_KPTS / 2); ++p)
                {
                    float px = std::max(std::min(pred_data[p * 2], (float)(img_w)), 0.f);
                    float py = std::max(std::min(pred_data[p * 2 + 1], (float)(img_h)), 0.f);
                    now.pts[p] = cv::Point2f(px, py);
                }

                now.rect = cv::Rect(now.pts[0], now.pts[2]);
                now.conf = final_conf;
                now.color = col_id;
                now.type = cls_id;
                now.t_size = t_size;
                objects.push_back(now);
            }
        }
    }
    }
}
*/
