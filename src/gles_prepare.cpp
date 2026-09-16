#include "exgine/gpu.hpp"
namespace exgine {
bool OpenGLESRenderer::prepare(std::string& error) { return ensure_programs(error); }
}
