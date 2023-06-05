#ifndef CRH_2022_DETECTOR_HPP_
#define CRH_2022_DETECTOR_HPP_
#include "data.hpp"
#include "umt.hpp"
#include <thread>
#include <chrono>
#include <array>
#include <string>
#include <cstring>
#include <opencv2/opencv.hpp>
#include "log.hpp"


#ifdef NX
    #include "detector_nx.h"
#else
    #include "detector_vino.h"
#endif
class Detector {
   private:

    typedef std::vector<float> Data;
    Logger logger;

    int type; // 0 for armor, 1 for windmill
    int INPUT_W;
    int INPUT_H;
    int NUM_CLASSES;
    int NUM_COLORS;
    int NUM_KPTS;
    float NMS_THRESH;
    float BBOX_CONF_THRESH;
    float MERGE_THRESH;
    bool V8_MODE;
    std::vector<ml::OutLayer> output_layers;
    std::vector<float> anchors[4];
#ifdef NX
    Trt* net;
    std::string model;
    std::string engine;
    Data ipt;
    Data opt[3];
    void copyBlob(Data& from, Data& to);
#else
    InferenceEngine::Core ie;
    InferenceEngine::CNNNetwork network;
    InferenceEngine::ExecutableNetwork executable_network;
    InferenceEngine::InferRequest infer_request;
    std::string model_xml;
    std::string model_bin;
    std::string input_name;
    std::vector<std::string> output_names;
    void copyBlob(Data& blob, InferenceEngine::Blob::Ptr& ieBlob);
#endif
    //for merge_nms
    struct pick_merge_store{
        int id;
        std::vector<cv::Point2f> merge_pts;
        std::vector<float> merge_confs;
    };
    struct armor_compare{
        bool operator ()(const armor::Armor& a,const armor::Armor& b) {
            return a.conf > b.conf; 
        }
    };
    float calc_iou(const armor::Armor& a,const armor::Armor& b);
    void decode_outputs(const float *prob, std::vector<armor::Armor>& objects,
                               ml::OutLayer layer_info, const int img_w, const int img_h);
    std::vector<armor::Armor> do_nms(std::vector<armor::Armor>& objects);
    std::vector<armor::Armor> do_merge_nms(std::vector<armor::Armor>& objects);
    cv::Mat static_resize(cv::Mat& img);
    Detector(); // never
    Detector(const Detector&); // never
   public:
    Detector(std::string name, int type, int log_level, bool mode); // type==0: amor, type==1: wind;
    int infer_cnt = 0;
    double resize_tot=0, infer_tot=0, decode_tot=0, total_tot=0;
    std::vector<armor::Armor> detect(Data& blob);
    void draw(cv::Mat&, const std::vector<armor::Armor>&);
};

inline float sigmoid(float x) { return (1.0 / (1.0 + exp(-x))); }

/**
 * @brief calculate sigmoid values in an array
 * 
 * @param src pointer of source array
 * @param dst pointer of destination array
 * @param length number of values
 */
inline void sigmoid(const float *src, float *dst, int length) {
    for (int i = 0; i < length; ++i) {
        dst[i] = (1.0 / (1.0 + exp(-src[i])));
    }
}
#endif // CRH_2022_DETECTOR_HPP_