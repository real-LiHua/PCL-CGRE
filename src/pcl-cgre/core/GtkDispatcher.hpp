#pragma once

#include "pclcore/network/Dispatcher.hpp"

#include <functional>
#include <glib.h>

namespace pcl {

/**
 * GTK main-loop dispatcher.
 *
 * Posts callbacks to the GTK main thread via g_idle_add so that
 * fetcher results are always handled on the main thread where
 * UI updates are safe.
 */
class GtkDispatcher final : public pclcore::network::Dispatcher {
public:
    void dispatch(std::function<void()> cb) override
    {
        auto* fn = new std::function<void()>(std::move(cb));
        g_idle_add_full(
            G_PRIORITY_DEFAULT_IDLE,
            [](gpointer data) -> gboolean {
                auto* f = static_cast<std::function<void()>*>(data);
                (*f)();
                delete f;
                return G_SOURCE_REMOVE;
            },
            fn,
            nullptr);  // we self-manage delete inside the callback
    }
};

}  // namespace pcl
