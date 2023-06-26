#ifndef DETERMINE_H_
#define DETERMINE_H_

#include <vector>
#include <list>

#include "toe_json.hpp"
#include "toe_structs.hpp"

namespace toe
{
    class Determine
    {
    public:
        Determine() = default;
        ~Determine() = default;

        void Init(const toe::json_head & input_json, int mode);
        bool change(int mode);
        bool change(const toe::json_head & input_json, int mode);
        bool change(const toe::json_head & input_json);
        armor_data get_results(const std::vector<armor_data> & input_armor);
    private:
        // 计算装甲板的实际距离
        void get_armor_z(armor_data & armor_input);
        // 计算装甲板的像素距离
        double get_distence(float x0, float y0, float x1, float y1);
        // 选取最近的装甲板
        armor_data select_near_armor(std::vector<armor_data> input_armor_vector);
        // 当前模式和优先击打的装甲板
        int new_mode = 0;
        // 用于标记丢失了亮的装甲板的识别有多少帧
        size_t loss_fps = 0;
        // 配置参数
        determine_data para_;
        // 历史输出的装甲板
        std::list<armor_data> history_armor;
    };
}

#endif