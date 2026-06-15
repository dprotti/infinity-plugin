// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config.h"
#include "infinity.h"
#include <glib.h>
#include <cstring>

static void print_usage(const char* argv0) {
    g_print(
        "Usage: %s [OPTIONS]\n\n"
        "Options:\n"
        "  --help                 Show this help\n"
        "  --capture=<backend>    Audio capture backend: pipewire or pulseaudio (default)\n"
        "                         The backend must match the one selected at build time.\n"
        "                         Rebuild with -Dcapture-backend=<backend> to change it.\n\n"
        "Build-time backend: %s\n",
        argv0,
#if CAPTURE_BACKEND_PULSEAUDIO
        "pulseaudio"
#else
        "pipewire"
#endif
    );
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }

        const char* capture_prefix = "--capture=";
        if (std::strncmp(argv[i], capture_prefix, std::strlen(capture_prefix)) == 0) {
            const char* requested = argv[i] + std::strlen(capture_prefix);
#if CAPTURE_BACKEND_PULSEAUDIO
            const char* compiled = "pulseaudio";
#else
            const char* compiled = "pipewire";
#endif
            if (std::strcmp(requested, compiled) != 0) {
                g_critical(
                    "Requested capture backend '%s' but this binary was built with '%s'.\n"
                    "Rebuild with: meson setup build -Dcapture-backend=%s",
                    requested, compiled, requested);
                return 1;
            }
            // Matches the compiled backend — nothing else to do.
            continue;
        }

        g_warning("Unknown option: %s (try --help)", argv[i]);
    }

    StandaloneParams params{};
    Infinity infinity(params);

    while (!infinity.window_closed()) {
        g_usleep(50000);
    }

    return 0;
}
