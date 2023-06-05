#include "detector.hpp"

/**
 * @brief Construct a new Detector:: Detector object
 * 
 * @param name name of model file (without extension name)
 * @param type detect-mode (0 for amor, 1 for wind)
 * @param log_level level of logging
 */
Detector::Detector(std::string name, int _type, int log_level, bool mode):
type(_type), INPUT_W(640), INPUT_H(640), NUM_KPTS(8), 
NUM_CLASSES(_type?2:9), NUM_COLORS(4), V8_MODE(mode),
NMS_THRESH(param.detector_args.nms_thresh[_type]), BBOX_CONF_THRESH(param.detector_args.conf_thresh[_type]), MERGE_THRESH(param.detector_args.merge_thresh[_type]),
logger("detector")
{
    logger.info("start_init_network");

    // load anchors
    std::string config_path = name + ".toml";
    auto &&config = toml::parse(config_path);
    auto &&anchors_vector = toml::get<std::vector<std::vector<int>>>(config.at("anchors"));
    for(int i=0; i<4; ++i) { // TODO: check anchors nums
        anchors[i] = Data(anchors_vector[i].begin(), anchors_vector[i].end());
    }

#ifdef NX
    model = name + ".onnx";
    engine = name + ".cache";

    net = new Trt();
    net->SetLogLevel(log_level);
    net->EnableFP16();
    //TODO:  net->SetWorkpaceSize();
    if (!net->DeserializeEngine(engine))
    { // cache not found
        net->BuildEngine(model, engine);
    }

    ipt = Data(1 * 3 * INPUT_H * INPUT_W);
    int no = net->GetNbOutputBindings();
    if(!V8_MODE)//使用yolov5模型
    {
        int NUM_SIZES = 0;
        if(type==0 && param.detector_args.is_V5_1) {
            NUM_CLASSES = 8;
            NUM_SIZES = 2;
        }
        if(type==1) {
            NUM_CLASSES = 3; // sjtu 3 class
        }
        for (int i = 1; i <= no; ++i)
        { //TODO: only one input?
            nvinfer1::Dims dims = net->GetBindingDims(i);
            int stride_h = INPUT_H / dims.d[2];
            int stride_w = INPUT_W / dims.d[3];
            assert(stride_h == stride_w && "Invalid stride!");
            opt[i - 1] = Data(net->GetBindingSize(i) / sizeof(float));
            output_layers.push_back((ml::OutLayer){
                .idx = i,
                .stride = stride_h,
                .num_anchor = dims.d[1],
                .num_out = dims.d[4]});
            assert(dims.d[4] == 5 + 10 + NUM_CLASSES + NUM_COLORS + NUM_SIZES && "Output dimension wrong!");
            logger.info("found network output: {}, size:{}, stride: {}, na: {}, no: {}"
                        ,i,vecsize_to_string(dims),stride_h,dims.d[1],dims.d[4]);
        }
    }else
    {
        NUM_CLASSES = 8;
        for (int i = 1; i <= no; ++i)
        { //TODO: only one input?
            nvinfer1::Dims dims = net->GetBindingDims(i);
            opt[i - 1] = Data(net->GetBindingSize(i) / sizeof(float));
            output_layers.push_back((ml::OutLayer){
                .idx = i,
                .num_out = dims.d[2]});
            assert(dims.d[2] == NUM_KPTS + NUM_CLASSES + NUM_COLORS + 2 && "Output dimension wrong!");
            logger.info("found network output: {}, size:{}, no: {}"
                        ,i,vecsize_to_string(dims), dims.d[2]);
        }
    }
#else
    model_xml = name + ".xml";
    model_bin = name + ".bin";

//    ie.SetConfig({{CONFIG_KEY(CACHE_DIR), "../net_cache"}});
    network = ie.ReadNetwork(model_xml, model_bin);

    input_name = network.getInputsInfo().begin()->first;
    if(!V8_MODE)//使用yolov5模型
    {
        int NUM_SIZES = 0;
        if(type==0 && param.detector_args.is_V5_1) {
            NUM_CLASSES = 8;
            NUM_SIZES = 2;
        }
        if(type==1) {
            NUM_CLASSES = 3; // sjtu 3 class
        }
        for (auto iter : network.getOutputsInfo())
        {
            auto dims = iter.second->getDims();
            int stride_h = INPUT_H / dims[2];
            int stride_w = INPUT_H / dims[3];
            assert(stride_h == stride_w && "Invalid stride!");
            output_layers.push_back((ml::OutLayer){
                .idx = (int)output_names.size(),
                .stride = stride_h,
                .num_anchor = (int)dims[1],
                .num_out = (int)dims[4]});
            output_names.push_back(iter.first);
            // logger.sinfo("type {}, cls {}, color {}, size {}, no {}", type, NUM_CLASSES, NUM_COLORS, NUM_SIZES, dims[4]);
            assert(dims[4] == 5 + 10 + NUM_CLASSES + NUM_COLORS + NUM_SIZES && "Output dimension wrong!");
            iter.second->setPrecision(InferenceEngine::Precision::FP32);
        logger.info("found network output: {}, size:{}, stride: {}, na: {}, no: {}"
                        ,iter.first,vecsize_to_string(dims),stride_h,dims[1],dims[4]); 
        }
    }else
    {
        NUM_CLASSES = 8;
        for (auto iter : network.getOutputsInfo())
        {
            auto dims = iter.second->getDims();
            output_layers.push_back((ml::OutLayer){
                .idx = (int)output_names.size(),
                .num_out = (int)dims[2]});
            output_names.push_back(iter.first);
            assert(dims[2] == NUM_KPTS + NUM_CLASSES + NUM_COLORS + 2 && "Output dimension wrong!");
            iter.second->setPrecision(InferenceEngine::Precision::FP32);
            logger.info("found network output: {}, size:{}, no: {}"
                        ,iter.first,vecsize_to_string(dims),dims[2]); 
        }
    }
    executable_network = ie.LoadNetwork(network, "GPU");
    infer_request = executable_network.CreateInferRequest();
#endif
    logger.info("network_init_done");
}

/**
 * @brief main process of detection
 * 
 * @param blob blob data converted from source image
 * @return std::vector<Armor> detect result
 */
std::vector<armor::Armor> Detector::detect(Data &blob)
{
    auto resize_start = std::chrono::steady_clock::now();
    // cv::Mat pr_img = static_resize(img);
#ifdef NX
    copyBlob(blob, ipt);
#else
    InferenceEngine::Blob::Ptr ieBlob = infer_request.GetBlob(input_name);
    copyBlob(blob, ieBlob);
#endif
    auto infer_start = std::chrono::steady_clock::now();
#ifdef NX
    net->CopyFromHostToDevice(ipt, 0);
    net->Forward();
#else
    infer_request.Infer();
#endif

    auto decode_start = std::chrono::steady_clock::now();
    std::vector<armor::Armor> objects;
#ifdef NX
    for (auto layer : output_layers)
    {
        net->CopyFromDeviceToHost(opt[layer.idx - 1], layer.idx);
        decode_outputs(opt[layer.idx - 1].data(), objects, layer, INPUT_W, INPUT_H);
    }
#else
    for (auto layer : output_layers)
    {
        const InferenceEngine::Blob::Ptr output_blob =
            infer_request.GetBlob(output_names[layer.idx]);
        InferenceEngine::MemoryBlob::CPtr moutput =
            InferenceEngine::as<InferenceEngine::MemoryBlob>(output_blob);
        auto moutputHolder = moutput->rmap();
        const float *net_pred =
            moutputHolder.as<const InferenceEngine::PrecisionTrait<
                InferenceEngine::Precision::FP32>::value_type *>();

        decode_outputs(net_pred, objects, layer, INPUT_W, INPUT_H);
    }
#endif
    objects = do_merge_nms(objects);
    
    if(type == 1) {
        // remove cls==0
        std::remove_if(objects.begin(), objects.end(), [](const armor::Armor &armor) {
            return armor.type == 0;
        });
        std::for_each(objects.begin(), objects.end(), [](armor::Armor &armor) {
            armor.type = 2 - armor.type;
        });
    }

    auto end_time = std::chrono::steady_clock::now();
    
    using duration = std::chrono::duration<double, std::milli>;
    double resize_time = duration(infer_start - resize_start).count();
    double infer_time = duration(decode_start - infer_start).count();
    double decode_time = duration(end_time - decode_start).count();
    double total_time = duration(end_time - infer_start).count(); // no resize

    infer_cnt++;
    resize_tot += resize_time;
    infer_tot += infer_time;
    decode_tot += decode_time;
    total_tot += total_time;

    // draw_objects(img, objects);
    // logger.info("detected {} objects", objects.size());
    return objects;
}

/**
 * @brief resize a image without changing the original aspect ratio
 * 
 * @param img source image
 * @return cv::Mat result image
 */
cv::Mat Detector::static_resize(cv::Mat &img)
{
    float r = std::min(INPUT_W / (img.cols * 1.0), INPUT_H / (img.rows * 1.0));
    // r = std::min(r, 1.0f);
    int unpad_w = r * img.cols;
    int unpad_h = r * img.rows;
    cv::Mat re(unpad_h, unpad_w, CV_8UC3);
    cv::resize(img, re, re.size());
    // cv::Mat out(INPUT_W, INPUT_H, CV_8UC3, cv::Scalar(114, 114, 114));
    cv::Mat out(INPUT_H, INPUT_W, CV_8UC3, cv::Scalar(114, 114, 114));
    re.copyTo(out(cv::Rect(0, 0, re.cols, re.rows)));
    return out;
}

/**
 * @brief decode the output of network model
 * 
 * @param prob raw float point, network output
 * @param objects gain detected objects to it by @a push_back
 * @param layer_info information about the last layer, differs by output stride of YOLO
 * @param img_w width of original image
 * @param img_h height of original image
 */
void Detector::decode_outputs(const float *prob, std::vector<armor::Armor> &objects,
                              ml::OutLayer layer_info, const int img_w,
                              const int img_h)
{
    // std::vector<int> classIds;
    // std::vector<int> indices;
    // std::vector<float> confidences;
    // std::vector<cv::Rect> bboxes;
    float scale = std::min(INPUT_W / (img_w * 1.0), INPUT_H / (img_h * 1.0));

    if(!V8_MODE)
    {
        if(type==0 && param.detector_args.is_V5_1) {
            // std::vector<float> *l_anchor = &anchors[0];
            // if (layer_info.stride == 16)
            //     l_anchor = &anchors[1];
            // if (layer_info.stride == 32)
            //     l_anchor = &anchors[2];
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

            // std::cout << "stride: " << layer_info.stride << std::endl;
            int out_h = INPUT_H / layer_info.stride;
            int out_w = INPUT_W / layer_info.stride;
            // std::cout << "size: " << out_h << " " << out_w << std::endl;
            float pred_data[layer_info.num_out];
            // [x, y, w, h, conf, (x,y)*5, hot(class), hot(color)]
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
        } else {
            // std::vector<float> *l_anchor = &anchors[0];
            // if (layer_info.stride == 16)
            //     l_anchor = &anchors[1];
            // if (layer_info.stride == 32)
            //     l_anchor = &anchors[2];
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

            // std::cout << "stride: " << layer_info.stride << std::endl;
            int out_h = INPUT_H / layer_info.stride;
            int out_w = INPUT_W / layer_info.stride;
            // std::cout << "size: " << out_h << " " << out_w << std::endl;
            float pred_data[layer_info.num_out];
            // [x, y, w, h, conf, (x,y)*5, hot(class), hot(color)]
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
                                    NUM_CLASSES + NUM_COLORS);
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

                            double final_conf =
                                obj_conf * sqrt(pred_data[15 + cls_id] *
                                                pred_data[15 + NUM_CLASSES + col_id]);
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
                                objects.push_back(now);
                            }
                        }
                    }
                }
            }
        }
    }else
    {
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

/**
 * @brief do Non-Maximum Suppression on detected objects, avoiding overlapped objects
 * 
 * @param objects all detected objects
 * @return std::vector<armor::Armor> result
 */
std::vector<armor::Armor> Detector::do_nms(std::vector<armor::Armor> &objects)
{
    std::vector<int> classIds;
    std::vector<int> indices;
    std::vector<float> confidences;
    std::vector<cv::Rect> bboxes;
    std::vector<armor::Armor> result;
    for (size_t i = 0; i < objects.size(); ++i)
    {
        bboxes.push_back(objects[i].rect);
        confidences.push_back(objects[i].conf);
        int cls_id = objects[i].color * 9 + objects[i].type;
        classIds.push_back(cls_id);
    }
    cv::dnn::NMSBoxes(bboxes, confidences, BBOX_CONF_THRESH, NMS_THRESH,
                      indices);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        result.push_back(objects[indices[i]]);
    }
    return result;
}


float Detector::calc_iou(const armor::Armor& a,const armor::Armor& b){
    cv::Rect_<float> inter = a.rect & b.rect;
    float inter_area = inter.area();
    float union_area = a.rect.area() + b.rect.area() - inter_area;
    return inter_area / union_area;
}

/**
 * @brief do Merge Non-Maximum Suppression on detected objects, avoiding overlapped objects
 * 
 * @param objects all detected objects
 * @return std::vector<armor::Armor> result
 */
std::vector<armor::Armor> Detector::do_merge_nms(std::vector<armor::Armor> &objects){
    std::vector<armor::Armor> result;
    std::vector<pick_merge_store> picked;

    std::sort(objects.begin(),objects.end(),armor_compare());
    for(int i = 0;i < objects.size();++i){
        armor::Armor& now = objects[i];
        bool keep = 1;
        for(int j = 0;j < picked.size();++j){
            armor::Armor& pre = objects[picked[j].id];
            float iou = calc_iou(now,pre);
            
            //store for merge_nms
            if(iou > NMS_THRESH || isnan(iou)){
                keep = 0;
                if(iou > MERGE_THRESH && now.color == pre.color && now.type == pre.type && now.t_size == pre.t_size){
                    picked[j].merge_confs.push_back(now.conf);
                    for(int k = 0;k < 5; ++k){
                        picked[j].merge_pts.push_back(now.pts[k]);
                    }
                }
                break;
            }
        }
        if(keep){
            picked.push_back({i,{},{}});
        }
    }
    for(int i = 0;i < picked.size();++i){
        int merge_num = picked[i].merge_confs.size();
        armor::Armor now = objects[picked[i].id];
        double conf_sum = now.conf;
        for(int j = 0;j < 5;++j) now.pts[j] *= now.conf;
        for(int j = 0;j < merge_num;++j){
            for(int k = 0;k < 5;++k){
                now.pts[k] += picked[i].merge_pts[j * 5 + k] * picked[i].merge_confs[j];
            }
            conf_sum += picked[i].merge_confs[j];
        }
        for(int j = 0;j < 5;++j) now.pts[j] /= conf_sum;
        result.emplace_back(now);
    }
    return result;
}
/**
 * @brief draw detection result on an image
 * 
 * @param bgr background image
 * @param objects objects to draw
 */
void Detector::draw(cv::Mat &bgr, const std::vector<armor::Armor> &objects)
{
if(!V8_MODE)
{
    static std::vector<const char*> type_names[2] = {
        {"0", "1", "2",  "3", "4", "5", "O", "Bs", "Bb"},
        {"U", "T"}
    };
    static std::vector<const char*> col_names[2] = {
        {"B", "R", "N", "P"},
        {"B", "R"}
    };

    for (size_t i = 0; i < objects.size(); i++)
    {
        const armor::Armor &obj = objects[i];

        char text[256];
        sprintf(text, "%d,%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.color,
                obj.type, obj.conf, obj.rect.x, obj.rect.y, obj.rect.width,
                obj.rect.height);
        logger.debug(text);

        cv::Scalar color = cv::Scalar(0, 1, 0);
        float c_mean = cv::mean(color)[0];
        cv::Scalar txt_color;
        if (c_mean > 0.5)
        {
            txt_color = cv::Scalar(0, 0, 0);
        }
        else
        {
            txt_color = cv::Scalar(255, 255, 255);
        }

        // cv::rectangle(image, obj.rect, color * 255, 2);
        if(type) { // windmill
            for (int p = 0; p < 5; ++p) {
                cv::line(bgr, obj.pts[p], obj.pts[(p + 1) % 5], color * 255, 2);
            }
        } else {
            for(int p = 0;p < 4;++p) {
                cv::line(bgr, obj.pts[p], obj.pts[(p + 1) % 4], color * 255, 2);
            }
        }
        // for (int p = 0; p < 5; ++p) {
        //     cv::circle(image, obj.pts[p], 2, cv::Scalar(0, 0, 255));
        // }

        sprintf(text, "%s%s %.1f%%", col_names[type][obj.color], type_names[type][obj.type],
                obj.conf * 100);

        int baseLine = 0;
        cv::Size label_size =
            cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseLine);

        cv::Scalar txt_bk_color = color * 0.7 * 255;

        int x = obj.rect.x;
        int y = obj.rect.y + 1;
        // int y = obj.rect.y - label_size.height - baseLine;
        if (y > bgr.rows)
            y = bgr.rows;
        // if (x + label_size.width > image.cols)
        // x = image.cols - label_size.width;

        cv::rectangle(
            bgr,
            cv::Rect(cv::Point(x, y),
                    cv::Size(label_size.width, label_size.height + baseLine)),
            txt_bk_color, -1);

        cv::putText(bgr, text, cv::Point(x, y + label_size.height),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, txt_color, 1);
    }
}else
{
    static std::vector<const char*> type_names[2] = {
        {"0", "1", "2",  "3", "4", "5", "6", "7"},
        {"U", "T"}
    };
    static std::vector<const char*> col_names[2] = {
        {"B", "R", "N", "P"},
        {"B", "R"}
    };
    static std::vector<const char*> t_size[2] = {
        {"S", "L"},
        {"W", "W"}
    };

    for (size_t i = 0; i < objects.size(); i++)
    {
        const armor::Armor &obj = objects[i];

        char text[256];
        sprintf(text, "%d,%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.color,
                obj.type, obj.conf, obj.rect.x, obj.rect.y, obj.rect.width,
                obj.rect.height);
        logger.debug(text);

        cv::Scalar color = cv::Scalar(0, 1, 0);
        float c_mean = cv::mean(color)[0];
        cv::Scalar txt_color;
        if (c_mean > 0.5)
        {
            txt_color = cv::Scalar(0, 0, 0);
        }
        else
        {
            txt_color = cv::Scalar(255, 255, 255);
        }

        // cv::rectangle(image, obj.rect, color * 255, 2);
        if(type) { // windmill
            for (int p = 0; p < 5; ++p) {
                cv::line(bgr, obj.pts[p], obj.pts[(p + 1) % 5], color * 255, 2);
            }
        } else {
            for(int p = 0;p < 4;++p) {
                cv::line(bgr, obj.pts[p], obj.pts[(p + 1) % 4], color * 255, 2);
            }
        }
        // for (int p = 0; p < 5; ++p) {
        //     cv::circle(image, obj.pts[p], 2, cv::Scalar(0, 0, 255));
        // }

        sprintf(text, "%s[%s]%s %.1f%%", col_names[type][obj.color], t_size[type][obj.t_size],
                type_names[type][obj.type], obj.conf * 100);

        int baseLine = 0;
        cv::Size label_size =
            cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseLine);

        cv::Scalar txt_bk_color = color * 0.7 * 255;

        int x = obj.rect.x;
        int y = obj.rect.y + 1;
        // int y = obj.rect.y - label_size.height - baseLine;
        if (y > bgr.rows)
            y = bgr.rows;
        // if (x + label_size.width > image.cols)
        // x = image.cols - label_size.width;

        cv::rectangle(
            bgr,
            cv::Rect(cv::Point(x, y),
                    cv::Size(label_size.width, label_size.height + baseLine)),
            txt_bk_color, -1);

        cv::putText(bgr, text, cv::Point(x, y + label_size.height),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, txt_color, 1);
    }
}
    return;
}
