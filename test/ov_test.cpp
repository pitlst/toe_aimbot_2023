#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

#include "string.h"

#include "debug.hpp"
#include "openvino/openvino.hpp"
// #include <inference_engine.hpp>

// static float calc_iou(const s_armor& a,const s_armor& b){
//     Rect_<float> inter = a.rect & b.rect;
//     float inter_area = inter.area();
//     float union_area = a.rect.area() + b.rect.area() - inter_area;
//     return inter_area / union_area;
// }

typedef struct
{
    cv::Point2f pts[5];

    float x_c;
    float y_c;
    float z;

    int t_size;
    int type;
    cv::Rect rect;
    float conf;
    int color;

} s_armor;

inline float sigmoid(float x) { return (1.0 / (1.0 + exp(-x))); }

/**
 * @brief calculate sigmoid values in an array
 *
 * @param src pointer of source array
 * @param dst pointer of destination array
 * @param length number of values
 */
inline void sigmoid(const float *src, float *dst, int length)
{
    for (int i = 0; i < length; ++i)
    {
        dst[i] = (1.0 / (1.0 + exp(-src[i])));
    }
}

int main()
{
    // 加载模型
    std::cout << "load network" << std::endl;
    ov::Core core;
    std::shared_ptr<ov::Model> model = core.read_model("/home/cheakf/Documents/toe_aimbot_2023/data/weights/1.xml", "/home/cheakf/Documents/toe_aimbot_2023/data/weights/1.bin");
    ov::CompiledModel compiled_model = core.compile_model(model, "GPU");

    // 生成anchors
    std::vector<float> stride_;
    std::vector<std::vector<float>> anchors;
    std::vector<float> temp;

    // 因为网络是8，32，16的排列，所以anchor对应的排列需要更改
    auto out_node = compiled_model.outputs();
    size_t out_tensor_size = out_node.size();
    for (auto out_n : out_node)
    {
        auto output_node = out_n.get_node();
        auto out_name = out_n.get_any_name();
        std::cout << out_name << std::endl;
        if (out_name == "stride_8")
        {
            temp = {4, 5, 8, 10, 13, 16};
            anchors.emplace_back(temp);
            stride_.emplace_back(8);
        }
        else if (out_name == "stride_16")
        {
            temp = {23, 29, 43, 55, 73, 105};
            anchors.emplace_back(temp);
            stride_.emplace_back(16);
        }
        else if (out_name == "stride_32")
        {
            temp = {146, 217, 231, 300, 335, 433};
            anchors.emplace_back(temp);
            stride_.emplace_back(32);
        }

        // std::cout << out_n.get_any_name() << std::endl;

        // std::cout << out_n.get_names().size() << std::endl;
        // for (auto out_n_name : out_n.get_names())
        // {
        //     std::cout << out_n_name << std::endl;
        // }
        // auto temp = out_n.get_shape();
        // for (auto ch : temp)
        // {
        //     std::cout << ch << " ";
        // }
        // std::cout << std::endl;
    }

    // throw std::logic_error("1");

    ov::InferRequest infer_request = compiled_model.create_infer_request();

    // 读取图像
    cv::VideoCapture capture;
    capture.open("/home/cheakf/Documents/toe_aimbot_2023/data/video/output_cut.mp4");
    cv::Mat frame;
    cv::Mat img3;
    std::vector<float> blob;

    while (capture.read(frame))
    {
        // 输入图像预处理
        std::cout << "read" << std::endl;
        cv::resize(frame, frame, cv::Size(640, 640));
        // 归一化
        img3.convertTo(frame, CV_32F, 1.0 / 255, 0);
        // 获取 Mat 中的数据指针
        const float *mat_data = img3.ptr<float>(0);
        // 将数据复制到 std::vector 中
        blob.assign(mat_data, mat_data + img3.total());
        // 提取模型输入的tensor的指针
        ov::Tensor input_tensor = infer_request.get_input_tensor();
        auto data1 = input_tensor.data<float>();
        // 复制数据
        std::memcpy(data1, blob.data(), sizeof(float) * blob.size());
        // 推理
        infer_request.infer();
        std::cout << "infer" << std::endl;

        float scale = 1;
        float img_w = 640;
        float img_h = 640;

        // 获取模型输出
        for (size_t i = 0; i < out_tensor_size; i++)
        {
            ov::Tensor output_tensor = infer_request.get_output_tensor(i);
            auto temp = output_tensor.get_shape();
            for (auto ch : temp)
            {
                std::cout << ch << " ";
            }
            std::cout << std::endl;
            const float *out_data = output_tensor.data<float>();
            int out_h = 640 / stride_[i];
            int out_w = 640 / stride_[i];
            float pred_data[29];
            int nums = 0;
            std::vector<float> *l_anchor = &anchors[i];
            // 后处理
            for (int na = 0; na < 3; ++na)
            {
                for (int h_id = 0; h_id < out_h; ++h_id)
                {
                    for (int w_id = 0; w_id < out_w; ++w_id)
                    {
                        int data_idx = (na * out_h * out_w + h_id * out_w + w_id) * 29;
                        float obj_conf = sigmoid(out_data[data_idx + 4]);
                        if (obj_conf > 0.6)
                        {
                            // std::cout << obj_conf << std::endl;
                            sigmoid(out_data + data_idx, pred_data, 5);
                            sigmoid(out_data + data_idx + 15, pred_data + 15,
                                    8 + 4 + 2);
                            memcpy(pred_data + 5, out_data + data_idx + 5,
                                   sizeof(float) * 10);
                            int col_id = std::max_element(pred_data + 15 + 8,
                                                          pred_data + 15 + 8 +
                                                              4) -
                                         (pred_data + 15 + 8);
                            if (col_id == 0)
                                continue;
                            int cls_id =
                                std::max_element(pred_data + 15,
                                                 pred_data + 15 + 8) -
                                (pred_data + 15);

                            int t_size = std::max_element(pred_data + 15 + 8 + 4,
                                                          pred_data + 15 + 8 + 4 + 2) -
                                         (pred_data + 15 + 8 + 4);

                            double final_conf =
                                obj_conf * pow(pred_data[15 + cls_id] *
                                                   pred_data[15 + 8 + col_id] *
                                                   pred_data[15 + 8 + 4 + t_size],
                                               1 / 3.);
                            if (final_conf > 0.6)
                            {
                                nums++;
                                // std::cout << final_conf << " " << col_id << " "
                                //           << cls_id << std::endl;

                                s_armor now;
                                float x = (pred_data[0] * 2.0 - 0.5 + w_id) *
                                          stride_[i];
                                float y = (pred_data[1] * 2.0 - 0.5 + h_id) *
                                          stride_[i];
                                float w =
                                    pow(pred_data[2] * 2, 2) * l_anchor->at(na * 2);
                                float h =
                                    pow(pred_data[3] * 2, 2) * l_anchor->at(na * 2 + 1);

                                for (int p = 0; p < 5; ++p)
                                {
                                    float px =
                                        (pred_data[5 + p * 2] * l_anchor->at(na * 2) +
                                         w_id * stride_[i]) /
                                        scale;
                                    float py = (pred_data[5 + p * 2 + 1] *
                                                    l_anchor->at(na * 2 + 1) +
                                                h_id * stride_[i]) /
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

                                now.x_c = (x0 + x1) / 2;
                                now.y_c = (y0 + y1) / 2;
                                now.rect = cv::Rect(x0, y0, x1 - x0, y1 - y0);
                                now.conf = final_conf;
                                now.color = col_id;
                                now.type = cls_id;
                                now.t_size = t_size;
                                // objects.armor.push_back(now);
                            }
                        }
                    }
                }
            }
        }

        // output_nms_.clear();
        // std::vector<pick_merge_store> picked;

        // std::sort(outputs_.armor.begin(), outputs_.armor.end(), armor_compare());
        // for (int i = 0; i < outputs_.armor.size(); ++i)
        // {
        //     s_armor &now = outputs_.armor[i];
        //     bool keep = 1;
        //     for (int j = 0; j < picked.size(); ++j)
        //     {
        //         s_armor &pre = outputs_.armor[picked[j].id];
        //         float iou = calc_iou(now, pre);

        //         // store for merge_nms
        //         if (iou > param_.nms_thresh || isnan(iou))
        //         {
        //             keep = 0;
        //             if (iou > param_.merge_thresh && now.color == pre.color && now.type == pre.type && now.t_size == pre.t_size)
        //             {
        //                 picked[j].merge_confs.push_back(now.conf);
        //                 for (int k = 0; k < 5; ++k)
        //                 {
        //                     picked[j].merge_pts.push_back(now.pts[k]);
        //                 }
        //             }
        //             break;
        //         }
        //     }
        //     if (keep)
        //     {
        //         picked.push_back({i, {}, {}});
        //     }
        // }
        // for (int i = 0; i < picked.size(); ++i)
        // {
        //     int merge_num = picked[i].merge_confs.size();
        //     s_armor now = outputs_.armor[picked[i].id];
        //     double conf_sum = now.conf;
        //     for (int j = 0; j < 5; ++j)
        //         now.pts[j] *= now.conf;
        //     for (int j = 0; j < merge_num; ++j)
        //     {
        //         for (int k = 0; k < 5; ++k)
        //         {
        //             now.pts[k] += picked[i].merge_pts[j * 5 + k] * picked[i].merge_confs[j];
        //         }
        //         conf_sum += picked[i].merge_confs[j];
        //     }
        //     for (int j = 0; j < 5; ++j)
        //         now.pts[j] /= conf_sum;
        //     output_nms_.emplace_back(now);
        // }
    }

    // InferenceEngine::Core ie_;
    // InferenceEngine::CNNNetwork network_ = ie_.ReadNetwork("/home/cheakf/Documents/toe_aimbot_2023/data/weights/1.xml","/home/cheakf/Documents/toe_aimbot_2023/data/weights/1.bin");
    // std::cout << network_.getInputsInfo().begin()->first << std::endl;

    return 0;
}