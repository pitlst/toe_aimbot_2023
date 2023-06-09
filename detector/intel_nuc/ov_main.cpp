#include <iostream>
#include "ov_detect.h"
#include "detector.h"
#include "common_structs.h"

using namespace std;

int main()
{
    Appconfig config;
    string json_path = "../../settings.json";
    load_config(config, json_path);
    OvO_Detector ov_detector;
    ov_detector.Init(config);
    return 0;
}
