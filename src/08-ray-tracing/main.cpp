#include <exception>
#include <iostream>
#include <string>

#define main SDL_main
#include "../common/rt/rt.h"
#include "../common/rt/ppg.h"

int main(int argc, char** argv) {
    try {
        RTApp app("Ray Tracing", 1600, 900, true);
        app.__ppg_on = true;
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}