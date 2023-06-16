#include <iostream>
#include <string>

#include "toe_json.hpp"
#include "toe_serial.hpp"
#include "grab_img.hpp"
#include "detect.hpp"

int main()
{
    std::cout << "hello toe" << std::endl;
#ifdef OPENVINO_START
    std::cout << "hello openvino" << std::endl;
#endif
#ifdef TENSORRT_START
    std::cout << "hello tensorrt" << std::endl;
#endif
    return 0;
}