#include <iostream>
#include <string>

#include "exgine/compiler.hpp"
#include "exgine/runtime.hpp"
#include "exgine/version.hpp"
#include "exgine/world.hpp"

int main() {
    const exgine::SourceText source(R"(
        game "Hello EXGINE" {
            world {
                terrain {
                    type = procedural
                    amplitude = 80
                    chunk_size = 32
                    resolution = 24
                    seed = 1337
                    sea_level = 0
                    vegetation_density = 24
                }
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

    exgine::Runtime runtime;
    if (!runtime.load(*result.ir)) {
        std::cerr << "failed to load compiled world\n";
        return 1;
    }
    if (!runtime.stream_world({0.0f, 0.0f, 0.0f}, 1)) {
        std::cerr << "failed to stream world\n";
        return 1;
    }

    std::cout << "EXGINE " << exgine::version_string << "\n";
    std::cout << "compiled world nodes = " << result.ir->root.children.size() << "\n";
    std::cout << "height(0,0) = " << runtime.world()->sample_height(0.0f, 0.0f) << "\n";
    std::cout << "loaded chunks = " << runtime.world_streamer()->loaded_count() << "\n";
    return 0;
}
