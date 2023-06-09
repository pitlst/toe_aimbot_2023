#include <json/json.h>
#include <string>
#include <fstream>
#include <iostream>
#include "../common_structs.h"

using namespace std;

void load_config(Appconfig& config, std::string json_file_path)
{
    Json::Reader reader;
    Json::Value value;
    ifstream in(json_file_path, ios::binary);
    cout << "load json now..." << endl;
    if (!in.is_open())
    {
        cerr << "Failed to open file: " << json_file_path;
        exit(1);
    }
    if (reader.parse(in, value))
    {
        config.base_config.camp = value["camp"].asInt();
        #ifdef USE_NVIDIA
        config.detect_config.engine_file_path = value["engine_file_path"].asString();
        #else
        config.detect_config.bin_file_path = value["bin_file_path"].asString();
        config.detect_config.xml_file_path = value["xml_file_path"].asString();
        #endif
        config.detect_config.batch_size = value["NCHW"]["batch_size"].asInt();
        config.detect_config.c = value["NCHW"]["C"].asInt();
        config.detect_config.w = value["NCHW"]["W"].asInt();
        config.detect_config.h = value["NCHW"]["H"].asInt();

        config.detect_config.type = value["img"]["type"].asInt();
        config.detect_config.width = value["img"]["width"].asInt();
        config.detect_config.height = value["img"]["height"].asInt();

        config.detect_config.nms_thresh = value["thresh"]["nms_thresh"].asFloat();
        config.detect_config.bbox_conf_thresh = value["thresh"]["bbox_conf_thresh"].asFloat();
        config.detect_config.merge_thresh = value["thresh"]["merge_thresh"].asFloat();

        config.detect_config.classes = value["nums"]["classes"].asInt();
        config.detect_config.classes = value["nums"]["sizes"].asInt();
        config.detect_config.classes = value["nums"]["colors"].asInt();
    }
    else
    {
        cerr << "Load Json Error!!!" << endl;
        exit(1);
    }
    cout << "load json success" << endl;
}
