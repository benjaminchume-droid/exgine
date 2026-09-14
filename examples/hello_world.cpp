#include <iostream>
#include <string>

#include "exgine/compiler.hpp"
#include "exgine/version.hpp"
#include "exgine/world.hpp"

int main() {
    const exgine::SourceText source(R"(
        game "Hello EXGINE" {
            world {
                terrain { type = procedural height = 20 }
                building "house" { floors = 2 }
            }
        }
    )");

    const auto result = exgine::Compiler{}.compile(source);
    if (!result.succeeded()) {
        for (const auto& diagnostic : result.diagnostics.all())
            std::cerr << diagnostic.location.line << ':' << diagnostic.location.column
                      << ": " << diagnostic.message << '\n';
        return 1;
    }

    const exgine::TerrainConfig terrain_config{20, 1337, 32.0f};
    const exgine::ProceduralWorld world(terrain_config);

    std::cout << "EXGINE " << exgine::version_string << "\n";
    std::cout << "compiled world nodes = " << result.ir->root.children.size() << "\n";
    std::cout << "height(0,0) = " << world.sample_height(0, 0) << "\n";
    std::cout << "height(32,32) = " << world.sample_height(32, 32) << "\n";
    return 0;
}
