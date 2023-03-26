#include <exception>
#include <iostream>

#define main SDL_main
#include "IAApp.h"

int main(int argc, char **argv) {
    try {
        IAApp app("subpass-input_attachment", 1200, 900, true);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
}