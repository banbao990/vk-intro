#include <exception>
#include <iostream>

#define main SDL_main
#include "CAApp.h"

int main(int argc, char **argv) {
    try {
        CAApp app("Chromatic-Aberration", 1200, 900, true);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}