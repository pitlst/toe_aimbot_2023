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
    ifstream in(json_file_name, ios::binary);
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
        config.detect_config.path.engine_file_path = value["engine_file_path"].asString();
        #else
        config.detect_config.path.bin_file_path = value["bin_file_path"];
        config.detect_config.path.xml_file_path = value["xml_file_path"];
        #endif
    }
    else
    {
        cerr << "Load Json Error!!!" << endl;
        exit(1);
    }
    cout << "load json success" << endl;
}