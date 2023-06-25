#include "determine.hpp"

void toe::Determine::Init(const toe::json_head &input_json, int mode)
{
    change(input_json, mode);
}

armor_data toe::Determine::get_results(const std::vector<armor_data> &input_armor)
{
    // 准备输出的装甲板
    armor_data rst;
    rst.x_c = -1;
    rst.y_c = -1;
    rst.z = -1;
    rst.type = 0;
    // 图像中心
    int x_c = para_.width / 2;
    int y_c = para_.height / 2;

    // 是否有红蓝颜色装甲板的输入
    bool have_light = false;
    // 是否有历史输出装甲板
    bool have_history = false;
    if (input_armor.size() > 0)
    {
        // 按类别分类输入的装甲板
        std::vector<armor_data> result[8];
        for (int i = 0; i < input_armor.size(); i++)
        {
            if (!have_light && (input_armor[i].color == 0 || input_armor[i].color == 1))
                have_light = true;
            result[input_armor[i].type].emplace_back(input_armor[i]);
        }
    }
    else
    {
        have_history = true;
    }
    
    // 如果有红蓝装甲板输入
    if (have_light)
    {
        // 根据模式来选择装甲板
        switch (new_mode)
        {
        // 电控自定义模式，使用电控传递来的目标
        case 1:
            
            break;
        // 前哨站模式，只找前哨站和基地
        case 2:
            /* code */
            break;
        // 默认的自瞄模式，寻找车辆装甲，优先英雄
        case 0:
        default:
            
            break;
        }
    }
    
    return rst;
}

bool toe::Determine::change(const toe::json_head &input_json, int mode)
{
    bool temp;
    temp = change(input_json);
    temp = change(mode);
    return temp;
}

bool toe::Determine::change(const toe::json_head &input_json)
{
    toe::json temp = input_json;
    para_.max_history_armor_len = temp["determine"]["max_history_armor_len"].Int();
    para_.max_loss_armor_fps = temp["determine"]["max_loss_armor_fps"].Int();
    para_.width = temp["NCHW"]["W"].Int();
    para_.height = temp["NCHW"]["H"].Int();
    para_.z_scale = temp["z_scale"].Int();
    return true;
}

bool toe::Determine::change(int mode)
{
    new_mode = mode;
    return true;
}

void toe::Determine::get_armor_z(armor_data & armor_input)
{
    if (armor_input.x_c != -1 && armor_input.y_c != -1)
    {
        cv::Point2f left_bottom = armor_input.pts[0];
        cv::Point2f left_top = armor_input.pts[1];
        cv::Point2f right_bottom = armor_input.pts[2];
        cv::Point2f right_top = armor_input.pts[3];

        double l_length = sqrt(pow(left_bottom.x * para_.width / 640 - left_top.x * para_.width / 640, 2) + pow(left_bottom.y * para_.height / 640 - left_top.y * para_.height / 640, 2));

        double r_length = sqrt(pow(left_bottom.x * para_.width / 640 - right_bottom.x * para_.width / 640, 2) + pow(left_bottom.y * para_.height / 640 - right_bottom.y * para_.height / 640, 2));

        if ((l_length + r_length) > 0)
            armor_input.z = para_.z_scale / (l_length + r_length);
        else
            armor_input.z = -1;
    }
}