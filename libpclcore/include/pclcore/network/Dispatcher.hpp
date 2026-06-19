#pragma once

#include <functional>

namespace pclcore::network {

/**
 * Abstract dispatcher interface.
 *
 * Fetcher worker threads call dispatch() to hand results back to the
 * application's "main" context.  The concrete implementation controls
 * where the callback runs:
 *
 *   - GtkDispatcher (GUI)  → g_idle_add → GTK main loop
 *   - SynchronousDispatcher (CLI) → inline, caller's thread
 */
class Dispatcher {
public:
    virtual ~Dispatcher() = default;

    /** Post @a cb to be executed in the application's main context. */
    virtual void dispatch(std::function<void()> cb) = 0;
};

/**
 * Synchronous dispatcher for CLI mode.
 *
 * Calls the callback immediately on the calling thread.  Used together
 * with std::promise / std::future so the main thread can block on the
 * result while the worker thread performs I/O.
 */
class SynchronousDispatcher final : public Dispatcher {
public:
    void dispatch(std::function<void()> cb) override { cb(); }
};

/**
 * Global dispatcher singleton.
 *
 * Defaults to a static SynchronousDispatcher instance so CLI mode
 * works without any explicit setup.  The GUI main() swaps it out
 * for a GtkDispatcher at startup.
 */
Dispatcher& get_dispatcher();
void set_dispatcher(Dispatcher& d);

}  // namespace pclcore::network
