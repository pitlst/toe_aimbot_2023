#include <iostream>
#include <string>

#include "toe_json.hpp"
#include "toe_serial.hpp"
#include "grab_img.hpp"
#include "detect.hpp"

int main()
{
    std::cout << PROJECT_PATH << std::endl;
#ifndef USE_NVIDIA
    std::cout << "hello openvino" << std::endl;
#else
    std::cout << "hello tensorrt" << std::endl;
#endif
    return 0;
}