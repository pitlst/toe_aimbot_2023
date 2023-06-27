#ifndef USE_NVIDIA

#include "ov_detect.hpp"

static float calc_iou(const armor_data &a, const armor_data &b)
{
    cv::Rect_<float> inter = a.rect & b.rect;
    float inter_area = inter.area();
    float union_area = a.rect.area() + b.rect.area() - inter_area;
    double iou = inter_area / union_area;
    if (std::isnan(iou))
    {
        iou = -1;
    }
    return iou;
}

void toe::Detector::init()
{
    std::cout << "load network" << std::endl;
    // 加载模型
    std::shared_ptr<ov::Model> model = core.read_model(std::string(PROJECT_PATH) + param_.xml_file_path, std::string(PROJECT_PATH) + param_.bin_file_path);
    ov::CompiledModel compiled_model = core.compile_model(model, "GPU");
    // 创建推理请求
    infer_request = compiled_model.create_infer_request();
    // 因为网络是8，32，16的排列，所以anchor对应的排列需要更改
    auto out_node = compiled_model.outputs();
    out_tensor_size = out_node.size();
    for (auto out_n : out_node)
    {
        auto out_name = out_n.get_any_name();
        if (out_name == "stride_8")
        {
            anchors.emplace_back(param_.a2);
            stride_.emplace_back(8);
        }
        else if (out_name == "stride_16")
        {
            anchors.emplace_back(param_.a3);
            stride_.emplace_back(16);
        }
        else if (out_name == "stride_32")
        {
            anchors.emplace_back(param_.a4);
            stride_.emplace_back(32);
        }
    }
    std::cout << "network_init_done. " << std::endl;
    blob.resize(param_.w * param_.h * 3);

    // throw std::logic_error("");
}

void toe::Detector::preprocess()
{
    // 提取模型输入的tensor的指针
    ov::Tensor input_tensor = infer_request.get_input_tensor();
    auto data1 = input_tensor.data<float>();
    // 输入图像预处理
    cv::resize(input_img, input_img, cv::Size(param_.w, param_.h));
    // 归一化
    int img_h = input_img.rows;
    int img_w = input_img.cols;
    float *blob_data = blob.data();
    size_t i = 0;
    for (size_t row = 0; row < img_h; ++row)
    {
        uchar *uc_pixel = input_img.data + row * input_img.step;
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
    // 复制数据
    std::memcpy(data1, blob.data(), sizeof(float) * blob.size());
}

void toe::Detector::inference()
{
    infer_request.infer();
}

void toe::Detector::postprocess()
{
    // 清除上一次推理的输出
    output_nms_.clear();
    // 解码网络输出
    for (size_t i = 0; i < out_tensor_size; i++)
    {
        // 获取输出tensor的指针
        ov::Tensor output_tensor = infer_request.get_output_tensor(i);
        const float *out_data = output_tensor.data<float>();

        int nums = 0;
        int now_stride = stride_[i];
        std::vector<float> *l_anchor = &(anchors[i]);
        int out_h = param_.h / now_stride;
        int out_w = param_.w / now_stride;
        int num_out = 5 + 10 + param_.classes + param_.colors + param_.sizes;
        float pred_data[num_out] = {0};
        // 图像三通道
        for (int na = 0; na < 3; ++na)
        {
            for (int h_id = 0; h_id < out_h; ++h_id)
            {
                for (int w_id = 0; w_id < out_w; ++w_id)
                {
                    pred_data[num_out] = {0};
                    int data_idx = (na * out_h * out_w + h_id * out_w + w_id) * num_out;
                    // 计算当前框的目标存在置信度
                    double obj_conf = toe::sigmoid(out_data[data_idx + 4]);
                    if (obj_conf > param_.bbox_conf_thresh)
                    {
                        toe::sigmoid(out_data + data_idx, pred_data, 5);
                        toe::sigmoid(out_data + data_idx + 15, pred_data + 15, param_.classes + param_.colors + param_.sizes);
                        std::memcpy(pred_data + 5, out_data + data_idx + 5, sizeof(float) * 10);

                        // // std::cout << pred_data[0] << std::endl;
                        // // std::cout << pred_data[1] << std::endl;
                        // // std::cout << pred_data[2] << std::endl;
                        // // std::cout << pred_data[3] << std::endl;
                        // // obj概率
                        // std::cout << "obj" << std::endl;
                        // std::cout << pred_data[4] << std::endl;
                        // // std::cout << pred_data[5] << std::endl;
                        // // std::cout << pred_data[6] << std::endl;
                        // // std::cout << pred_data[7] << std::endl;
                        // // std::cout << pred_data[8] << std::endl;
                        // // std::cout << pred_data[9] << std::endl;
                        // // std::cout << pred_data[10] << std::endl;
                        // // std::cout << pred_data[11] << std::endl;
                        // // std::cout << pred_data[12] << std::endl;
                        // // std::cout << pred_data[13] << std::endl;
                        // // std::cout << pred_data[14] << std::endl;
                        // // 对应类别概率
                        // std::cout << "classes" << std::endl;
                        // std::cout << pred_data[15] << std::endl;
                        // std::cout << pred_data[16] << std::endl;
                        // std::cout << pred_data[17] << std::endl;
                        // std::cout << pred_data[18] << std::endl;
                        // std::cout << pred_data[19] << std::endl;
                        // std::cout << pred_data[20] << std::endl;
                        // std::cout << pred_data[21] << std::endl;
                        // std::cout << pred_data[22] << std::endl;
                        // // 对应颜色概率
                        // std::cout << "color" << std::endl;
                        // std::cout << pred_data[23] << std::endl;
                        // std::cout << pred_data[24] << std::endl;
                        // std::cout << pred_data[25] << std::endl;
                        // std::cout << pred_data[26] << std::endl;
                        // // 对应大小概率
                        // std::cout << "size" << std::endl;
                        // std::cout << pred_data[27] << std::endl;
                        // std::cout << pred_data[28] << std::endl;

                        // throw std::logic_error("");
                        // 计算当前框的颜色
                        int col_id = std::max_element(pred_data + 15 + param_.classes,
                                                      pred_data + 15 + param_.classes +
                                                          param_.colors) -
                                     (pred_data + 15 + param_.classes);

                        // std::cout << "col_id is " << col_id << std::endl;
                        // 颜色不同停止计算
                        if (col_id == param_.camp)
                        {
                            continue;
                        }

                        // std::cout << "color" << std::endl;
                        // std::cout << col_id << std::endl;
                        // std::cout << param_.classes << std::endl;
                        // std::cout << pred_data[23] << std::endl;
                        // std::cout << pred_data[24] << std::endl;
                        // std::cout << pred_data[25] << std::endl;
                        // std::cout << pred_data[26] << std::endl;

                        // 计算当前框的类别
                        int cls_id = std::max_element(pred_data + 15, pred_data + 15 + param_.classes) - (pred_data + 15);
                        // 计算是否是大小装甲
                        int t_size = std::max_element(pred_data + 15 + param_.classes + param_.colors, pred_data + 15 + param_.classes + param_.colors + 2) - (pred_data + 15 + param_.classes + param_.colors);

                        // 计算当前框的最终置信度
                        // std::cout << pred_data[15 + param_.classes + 0] << std::endl;
                        // std::cout << pred_data[15 + param_.classes + 1] << std::endl;
                        // std::cout << pred_data[15 + param_.classes + 2] << std::endl;
                        // std::cout << pred_data[15 + param_.classes + 3] << std::endl;
                        // std::cout << pred_data[15 + param_.classes + param_.colors + 0] << std::endl;
                        // std::cout << pred_data[15 + param_.classes + param_.colors + 1] << std::endl;

                        // std::cout << pred_data[15 + cls_id] << std::endl;
                                                // std::cout << "color" << std::endl;
                        // std::cout << pred_data[23] << std::endl;
                        // std::cout << pred_data[24] << std::endl;
                        // std::cout << pred_data[25] << std::endl;
                        // std::cout << pred_data[26] << std::endl;
                        // std::cout << "color is " << pred_data[15 + param_.classes + col_id]  * 100 << std::endl;
                        // std::cout << pred_data[15 + param_.classes + param_.colors + t_size] * 100 << std::endl;

                        double final_conf = obj_conf * std::pow(pred_data[15 + cls_id] *
                                                                    pred_data[15 + param_.classes + col_id] *
                                                                    pred_data[15 + param_.classes + param_.colors + t_size],
                                                                1 / 3.);

                        // double final_conf = obj_conf * pred_data[15 + cls_id];
                        if (final_conf > param_.bbox_conf_thresh)
                        {
                            nums++;
                            armor_data now;
                            float x = (pred_data[0] * 2.0 - 0.5 + w_id) * now_stride;
                            float y = (pred_data[1] * 2.0 - 0.5 + h_id) * now_stride;
                            float w = std::pow(pred_data[2] * 2, 2) * l_anchor->at(na * 2);
                            float h = std::pow(pred_data[3] * 2, 2) * l_anchor->at(na * 2 + 1);
                            for (int p = 0; p < 5; ++p)
                            {
                                float px = (pred_data[5 + p * 2] * l_anchor->at(na * 2) + w_id * now_stride);
                                float py = (pred_data[5 + p * 2 + 1] * l_anchor->at(na * 2 + 1) + h_id * now_stride);
                                px = std::max(std::min(px, (float)(param_.w)), 0.f);
                                py = std::max(std::min(py, (float)(param_.h)), 0.f);
                                now.pts[p] = cv::Point2f(px, py);
                            }
                            float x0 = x - w * 0.5;
                            float y0 = y - h * 0.5;
                            float x1 = x + w * 0.5;
                            float y1 = y + h * 0.5;

                            // 检查输出目标的上下界，应当在640和0之间
                            x0 = std::max(std::min(x0, (float)(param_.w)), 0.f);
                            y0 = std::max(std::min(y0, (float)(param_.h)), 0.f);
                            x1 = std::max(std::min(x1, (float)(param_.w)), 0.f);
                            y1 = std::max(std::min(y1, (float)(param_.h)), 0.f);

                            now.x_c = (x0 + x1) / 2;
                            now.y_c = (y0 + y1) / 2;
                            now.rect = cv::Rect(x0, y0, x1 - x0, y1 - y0);
                            now.conf = final_conf;
                            now.color = col_id;
                            now.type = cls_id;
                            now.t_size = t_size;
                            output_nms_.emplace_back(now);
                        }
                    }
                }
            }
        }
    }

    std::cout << "output_nms_ is " << output_nms_.size() << std::endl;

    // nms去除重叠装甲板
    outputs_armor.clear();
    std::vector<pick_merge_store> picked;
    std::sort(output_nms_.begin(), output_nms_.end(), [](const armor_data &a, const armor_data &b){ return a.conf > b.conf; });
    for (int i = 0; i < output_nms_.size(); ++i)
    {
        armor_data &now = output_nms_[i];
        bool keep = true;
        for (int j = 0; j < picked.size(); ++j)
        {
            armor_data &pre = output_nms_[picked[j].id];
            float iou = calc_iou(now, pre);
            if (iou > param_.nms_thresh)
            {
                keep = false;
                if (now.color == pre.color && now.type == pre.type && now.t_size == pre.t_size && iou > param_.merge_thresh)
                {
                    picked[j].merge_confs.push_back(now.conf);
                    for (int k = 0; k < 5; ++k)
                    {
                        picked[j].merge_pts.push_back(now.pts[k]);
                    }
                }
                break;
            }
        }
        if (keep)
        {
            picked.push_back({i, {}, {}});
        }
    }

    // 根据置信度对关键点做加权平均，矫正关键点位置
    for (int i = 0; i < picked.size(); ++i)
    {
        int merge_num = picked[i].merge_confs.size();
        armor_data now = output_nms_[picked[i].id];
        double conf_sum = now.conf;
        for (int j = 0; j < 5; ++j)
        {
            now.pts[j] *= now.conf;
        }
        for (int j = 0; j < merge_num; ++j)
        {
            for (int k = 0; k < 5; ++k)
            {
                now.pts[k] += picked[i].merge_pts[j * 5 + k] * picked[i].merge_confs[j];
            }
            conf_sum += picked[i].merge_confs[j];
        }
        for (int j = 0; j < 5; ++j)
        {
            now.pts[j] /= conf_sum;
        }
        outputs_armor.emplace_back(now);
    }
}

bool toe::Detector::detect()
{
    if (!input_imgs.empty())
    {
        // std::cout << "read" << std::endl;
        img_mutex_.lock();
        input_img = input_imgs.back();
        input_imgs.clear();
        img_mutex_.unlock();

        // std::cout << "trans" << std::endl;
        preprocess();
        // std::cout << "infer" << std::endl;
        inference();
        // std::cout << "post" << std::endl;
        postprocess();
    }
    return true;
}
#endif