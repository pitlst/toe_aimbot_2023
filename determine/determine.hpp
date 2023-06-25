#ifndef DETERMINE_H_
#define DETERMINE_H_

#include <vector>

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
        // 计算装甲板的距离
        void get_armor_z(armor_data & armor_input);
        // 当前模式和优先击打的装甲板
        int new_mode = 0;
        // 用于标记丢失了亮的装甲板的识别有多少帧
        size_t loss_fps = 0;
        // 配置参数
        determine_data para_;
        // 历史输出的装甲板
        std::vector<armor_data> history_armor;
    };
}

#endif