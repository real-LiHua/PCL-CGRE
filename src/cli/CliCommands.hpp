#pragma once

/**
 * Entry point for CLI mode.
 *
 * Parses argv and dispatches to the appropriate subcommand handler.
 * Returns an exit code (0 = success, non-zero = error).
 *
 * The default SynchronousDispatcher is active; fetcher callbacks fire
 * inline, and the CLI uses std::promise/future to block for results.
 */
int run_cli(int argc, char* argv[]);
