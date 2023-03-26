#include <exception>
#include <iostream>

#define main SDL_main
#include "triVertexApp.h"

int main(int argc, char **argv) {
    try {
        TriVertexApp app("vertex", 800, 600, true);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}