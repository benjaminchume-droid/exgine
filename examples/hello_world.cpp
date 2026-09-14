#include <iostream>

#include "exgine/ir.hpp"
#include "exgine/world.hpp"

int main() {
    exgine::IR game;
    game.add_property("terrain", std::string("procedural"));
    game.add_property("terrain_height", int64_t{20});

    exgine::ProceduralWorld world({20, 1337, 32.0f});

    std::cout << "EXGINE 0.1 prototype\n";
    std::cout << "terrain = procedural\n";
    std::cout << "height(0,0) = " << world.sample_height(0, 0) << "\n";
    std::cout << "height(32,32) = " << world.sample_height(32, 32) << "\n";

    return 0;
}
