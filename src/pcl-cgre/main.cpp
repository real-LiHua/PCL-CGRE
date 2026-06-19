#include <cstdio>
#include <cstring>

#include "app/App.hpp"
#include "core/GtkDispatcher.hpp"
#include "pclcore/network/Dispatcher.hpp"
#include "CliCommands.hpp"

int main(int argc, char* argv[])
{
    /* ── GUI mode ──────────────────────────────────────────────────── */
    if (argc > 1 && std::strcmp(argv[1], "--gui") == 0) {
        // Inject GtkDispatcher so that fetcher callbacks arrive on the
        // GTK main thread via g_idle_add.
        pcl::GtkDispatcher gtk_disp;
        pclcore::network::set_dispatcher(gtk_disp);

        PclApplication app;
        // Pass only argv[0] so GTK doesn't choke on unrecognized flags.
        return app.run(1, argv);
    }

    /* ── CLI mode (default) ────────────────────────────────────────── */
    // The global SynchronousDispatcher is already active (set at static
    // init time in libpclcore).  Fetcher callbacks fire inline on the
    // worker thread, and CLI commands use std::promise / future to
    // block for results.
    return run_cli(argc, argv);
}
