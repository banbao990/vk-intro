#include <exception>
#include <iostream>

#define main SDL_main
#include "fixedTriangle.h"

int main(int argc, char **argv) {
    try {
        FixedTriangleApp app("Hello Triangle", 800, 600, true);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}