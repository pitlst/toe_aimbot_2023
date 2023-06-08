/*
* common_structs.h
* Created on: 20230605
* Author: sumang
* Description: some common structs
*/
#ifndef COMMON_STRUCTS_H_
#define COMMON_STRUCTS_H_

typedef struct 
{
    struct path
    {
        string engine_file_path;
        string bin_file_path;
        string xml_file_path;
    };

    struct NCHW
    {
        int batch_size;
        int h;
        int w;
        int c;
    };

    struct img
    {
        int type; // rgb, bgr, yuv, bayerrg8 ...
        int width;
        int height;
    };

    struct thresh
    {
        float nms_thresh;
        float bbox_conf_thresh;
        float merge_thresh;
    };

    struct nums
    {
        int classes;
        int colors;
        int sizes;
    };

    
}s_detector_params;

typedef struct
{
    int camp;
}s_base_params;

typedef struct
{
    s_detector_params detect_config;
    s_base_params base_config;
}Appconfig;

#endif