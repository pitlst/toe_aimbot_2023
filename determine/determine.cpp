#include "determine.hpp"

#include <limits.h>

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

    // 是否有红蓝颜色装甲板的输入
    bool have_light = false;
    // 按类别分类输入的装甲板
    std::vector<armor_data> result[8];
    if (input_armor.size() > 0)
    {
        for (int i = 0; i < input_armor.size(); i++)
        {
            if (!have_light && (input_armor[i].color == 0 || input_armor[i].color == 1))
                have_light = true;
            result[input_armor[i].type].emplace_back(input_armor[i]);
        }
    }
    if (have_light)
    {
        loss_fps = 0;
        // 根据模式来选择装甲板
        switch (new_mode)
        {
        // 默认的自瞄模式，寻找车辆装甲，优先英雄
        case 0:
            // 先找英雄
            if (result[1].size() > 0)
            {
                rst = select_near_armor(result[6]);
                history_armor.emplace_back(rst);
            }
            // 再找步兵
            else if (result[3].size() > 0 || result[4].size() > 0 || result[5].size() > 0)
            {
                std::vector<armor_data> temp = result[3];
                temp.insert(temp.end(), result[4].begin(), result[4].end());
                temp.insert(temp.end(), result[5].begin(), result[5].end());
                rst = select_near_armor(temp);
                history_armor.emplace_back(rst);
            }
            // 再找哨兵
            else if (result[0].size() > 0)
            {
                rst = select_near_armor(result[0]);
                history_armor.emplace_back(rst);
            }
            // 再找工程
            else if (result[2].size() > 0)
            {
                rst = select_near_armor(result[2]);
                history_armor.emplace_back(rst);
            }
            // 都没有就有啥算啥
            else
            {
                rst = select_near_armor(input_armor);
                history_armor.emplace_back(rst);
            }
            break;
        // 前哨站模式，只找前哨站和基地
        case 2:
            if (result[6].size() > 0)
            {
                rst = select_near_armor(result[6]);
                history_armor.emplace_back(rst);
            }
            else if (result[7].size() > 0)
            {
                rst = select_near_armor(result[7]);
                history_armor.emplace_back(rst);
            }
            break;
        // 电控自定义模式，使用电控传递来的目标
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            int armor_object = new_mode - 10;
            if (result[armor_object].size() > 0)
            {
                rst = select_near_armor(result[armor_object]);
                history_armor.emplace_back(rst);
            }
            break;
        }
    }
    else
    {
        // 只有存在历史装甲版才会继续输出目标
        if (history_armor.size() > 0)
        { 
            loss_fps++;           
            rst = history_armor.back();
            // 判断装甲板闪烁
            if (result[rst.type].size() > 0)
            {
                rst = select_near_armor(result[2]);
            }
            // 判断装甲板超时是否过长
            if (loss_fps > para_.max_loss_armor_fps)
            {
                history_armor.clear();
                loss_fps = 0;
            }
        }
    }
    // 获取装甲板测距
    if (rst.x_c != -1 && rst.y_c != -1)
    {
        cv::Point2f left_bottom = rst.pts[0];
        cv::Point2f left_top = rst.pts[1];
        cv::Point2f right_bottom = rst.pts[2];
        cv::Point2f right_top = rst.pts[3];

        double l_length = sqrt(pow(left_bottom.x * para_.width / 640 - left_top.x * para_.width / 640, 2) + pow(left_bottom.y * para_.height / 640 - left_top.y * para_.height / 640, 2));

        double r_length = sqrt(pow(left_bottom.x * para_.width / 640 - right_bottom.x * para_.width / 640, 2) + pow(left_bottom.y * para_.height / 640 - right_bottom.y * para_.height / 640, 2));

        if ((l_length + r_length) > 0)
            rst.z = para_.z_scale / (l_length + r_length);
        else
            rst.z = -1;
    }
    // 检查历史装甲板列表长度
    if (history_armor.size() > para_.max_history_armor_len)
    {
        history_armor.pop_front();
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
    history_armor.clear();
    loss_fps = 0; 
    return true;
}

double toe::Determine::get_distence(float x0, float y0, float x1, float y1)
{
    return sqrt(pow(x0 - x1, 2) + pow(y0 - y1, 2));
}

armor_data toe::Determine::select_near_armor(std::vector<armor_data> input_armor_vector)
{
    double last_distence = DBL_MAX;
    double distence = DBL_MIN;
    armor_data rst;
    int x_c, y_c;
    // 有历史选历史，没历史选屏幕中心
    if (history_armor.size() > 0)
    {
        x_c = history_armor.back().x_c;
        y_c = history_armor.back().y_c;
    }
    else
    {
        x_c = para_.width / 2;
        y_c = para_.height / 2;
    }
    for (auto & armor_ch : input_armor_vector)
    {
        distence = get_distence(x_c, y_c, armor_ch.x_c, armor_ch.y_c);
        if (last_distence > distence)
        {
            last_distence = distence;
            rst = armor_ch;
        }
    }
    return rst;
}