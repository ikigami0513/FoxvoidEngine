#include "core/Engine.hpp"
#include <iostream>

int main() {
    try {
        // Create an instance of the engine with a 800x600 window
        Engine engine(1920, 1080, "Foxvoid Engine");
        
        // Start the main game loop
        engine.Run();
        
    } catch (const std::exception& e) {
        // Catch and display any standard exceptions thrown during execution
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
