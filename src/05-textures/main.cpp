#include <exception>
#include <iostream>

#define main SDL_main
#include "meshTexApp.h"

int main(int argc, char **argv) {
    try {
        MeshTexApp app("texture", 1200, 900, true);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}

// TODO