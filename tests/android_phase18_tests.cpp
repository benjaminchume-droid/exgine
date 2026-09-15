#include "exgine/android.hpp"

#include <cassert>

int main() {
    using namespace exgine;

    AndroidEglConfig config;
    assert(config.valid());
    config.depth_bits = -1;
    assert(!config.valid());

    AndroidPresentationStats stats;
    assert(stats.valid());
    stats.attached = true;
    assert(!stats.valid());
    stats.width = 1920;
    stats.height = 1080;
    assert(stats.valid());

    AndroidEglPresenter presenter;
    assert(!presenter.ready());
    assert(!presenter.attach(nullptr));
    assert(!presenter.last_error().empty());
    assert(!presenter.resize());
    assert(!presenter.present(RenderFrame{}));
    presenter.detach();
    assert(!presenter.ready());
    assert(presenter.stats().valid());
    return 0;
}
