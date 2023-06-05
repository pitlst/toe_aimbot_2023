#include "detect_run.hpp"
#include "log.hpp"
#include "args.hpp"
extern args::Args param;

void img2blob(cv::Mat &img, std::vector<float> &dst)
{
    // cv::dnn::blobFromImage(img, 1./255, img.size, cv::Scalar(), true);
    // return;
    int img_h = img.rows;
    int img_w = img.cols;

    float *blob_data = dst.data();

    size_t i = 0;
    for (size_t row = 0; row < img_h; ++row)
    {
        uchar *uc_pixel = img.data + row * img.step;
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

/**
 * @brief a thread dealing with affairs about detecting
 * 
 * @paragraph detect_run processes
 *  load neural network model;
 *  receive image data from <sensor>;
 *  use different model according to current mode;
 *  detect objects in the image;
 *  publish the result to <predict>;
 * 
 */
void detect_run()
{
    Logger logger("detect");
    logger.debug("detect_run begin");
    umt::Subscriber<SENSOR_DATA_MSG> sensor_pub("sensor_data");
    umt::Publisher<DETECT_MSG> detect_am_pub("detect_am_data");
    umt::Publisher<DETECT_MSG> detect_swm_pub("detect_swm_data");
    umt::Publisher<DETECT_MSG> detect_bwm_pub("detect_bwm_data");
    umt::Publisher<HEART_BEAT> detect_hb_pub("Detect_HB");
    Detector *pdetector = nullptr;
    umt::Publisher<DETECT_MSG>* detect_pub=nullptr;
    Detector dtt4am(param.detector_args.path2model_am, 0, 0, param.detector_args.V8_MODE_am); // 针对装甲板的model
    logger.debug("model for armor, construct successfully");
    Detector dtt4wm(param.detector_args.path2model_wm, 1, 0, param.detector_args.V8_MODE_wm); // 针对风车的model
    logger.debug("model for windmill, construct successfully");

    std::string record_path;
    cv::VideoWriter vw;
    MODE mode;
    
    std::vector<float> blob(640*640*3);

    while ((mode=param.get_run_mode()) != HALT)
    {
        SENSOR_DATA_MSG msg;
        try {
            detect_hb_pub.push(HEART_BEAT{HEART_BEAT::TYPE::WAIT});
            msg = sensor_pub.pop();
            detect_hb_pub.push(HEART_BEAT{HEART_BEAT::TYPE::CONTD});
        } catch(const HaltEvent&) {
            break;
        }
        img2blob(msg.src, blob);
        mode = msg.run_mode;
        if (mode == AUTO_AIM || mode == ANTI_ROT) {
            detect_pub = &detect_am_pub;
            pdetector = &dtt4am;
        } else if(mode == B_WM) {
            detect_pub = &detect_bwm_pub;
            pdetector = &dtt4wm;
        } else if(mode == S_WM) {
            detect_pub = &detect_swm_pub;
            pdetector = &dtt4wm;
        } else {
            logger.warn("Unknown mode {}", mode);
            continue;
        }

        // logger.warn("detect_poped");
        // logger.warn("sensor_pub: pop");

        std::vector<armor::Armor> detections = pdetector->detect(blob);
        logger.sinfo("Detect {}", detections.size());

        if (param.debug.with_imshow_detector || param.detector_need) // draw if need imshow OR record
        {
            pdetector->draw(msg.src, detections);
        }
        if(param.detector_need && record_path == "") {
            record_path = param.detector_prefix + param.asc_begin_time + ".avi";
            // vw = cv::VideoWriter(record_path, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 25, cv::Size(msg.src.size[1], msg.src.size[0]));
            vw = cv::VideoWriter(record_path, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), param.detector_fps, cv::Size(msg.src.size[1], msg.src.size[0]));
            if(!vw.isOpened()) {
                logger.warn("[detector-record] fail to open file '{}', disable recording!", record_path);
                param.detector_need = false;
            }
        }
        if(param.detector_need) { // may be disabled due to `if(!vw.isOpened())`
            // cv::imwrite("../logs/tmp.jpg", frame);
            vw << msg.src;
        }
        // for(auto v: detections) {
        //     std::cout << v.color << ' ' << v.type << std::endl;
        //     for(int i=0; i<5; ++i)std::cout << v.pts[i] << ' ';
        // }
        // logger.warn("de{}", detections.size());
        detect_pub->push((DETECT_MSG){
            .mode = mode,
            .time_stamp = msg.time_stamp,
            .imu_data = msg.imu_data,
            .res = std::move(detections),
            .src = std::move(msg.src)
        });
        if(pdetector->infer_cnt % 100 == 0) {
            detect_hb_pub.push(HEART_BEAT{HEART_BEAT::TYPE::DEFAULT});
        }
        // logger.warn("detect_push");

        if(pdetector->infer_cnt % 1000 == 1) {
            logger.sinfo(
                "[Detector] rtime: {:.3f}ms, itime: {:.3f}ms, dtime: {:.3f}ms, ttime: {:.3f}ms, fps: {:.2f}",
                pdetector->resize_tot / pdetector->infer_cnt,
                pdetector->infer_tot / pdetector->infer_cnt,
                pdetector->decode_tot / pdetector->infer_cnt,
                pdetector->total_tot / pdetector->infer_cnt,
                pdetector->infer_cnt / pdetector->total_tot * 1000 // convert millisecond to second
            );
        }
    }
    if(vw.isOpened()) {
        vw.release();
    }
    logger.sinfo("[Detector] Exited.");
    return;
}