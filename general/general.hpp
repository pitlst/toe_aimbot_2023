#ifndef GENERAL_H_
#define GENERAL_H_

#include <math.h>
#include "toe_structs.hpp"

namespace toe
{
    inline void sigmoid(const float *src, float *dst, int length)
    {
        for (int i = 0; i < length; ++i)
        {
            dst[i] = (1.0 / (1.0 + std::exp(-src[i])));
        }
    }

    inline float sigmoid(float x)
    {
        return (1.0 / (1.0 +  std::exp(-x)));
    }

    inline float calc_iou(const armor_data &a, const armor_data &b)
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
}

#endif