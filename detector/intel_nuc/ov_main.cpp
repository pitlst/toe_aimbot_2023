#include <iostream>
#include <opencv2/opencv.hpp>
#include "ov_detect.h"
#include "detector.h"
#include "common_structs.h"

using namespace std;
using namespace cv;

int main()
{
    Appconfig config;
    string json_path = "../../settings.json";
    load_config(config, json_path);
    OvO_Detector ov_detector;
    ov_detector.Init(config);

    VideoCapture cap("test.mp4");
    if (!cap.isOpened())
    {
        cout << "Error opening video file" << endl;
        return -1;
    }
    double fps = cap.get(CAP_PROP_FPS);
    int total_frames = cap.get(CAP_PROP_FRAME_COUNT);
    namedWindow("Video", WINDOW_NORMAL);
    Mat frame;
    while (cap.read(frame))
    {
        imshow("Video", frame);
        ov_detector.push_img(frame);
        cout << ov_detector.detect() << endl;
        waitKey(1000/fps);
    }

    cap.release();
    destroyAllWindows();

    return 0;
}
