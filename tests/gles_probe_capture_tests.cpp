#include "exgine/gles_probe_capture.hpp"
#include <cassert>
#include <string>
int main(){assert(sizeof(exgine::ReflectionProbeCaptureTarget)>=sizeof(exgine::GlUInt)*3);assert(exgine::cube_faces().size()==6);return 0;}