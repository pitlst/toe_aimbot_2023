/*
* common_structs.h
* Created on: 20230605
* Author: sumang
* Description: some common structs
*/
#ifndef OV_DETECT_H_
#define OV_DETECT_H_

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
        int 
    };

    
}s_detector_params;
