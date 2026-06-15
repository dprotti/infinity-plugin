// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Audio capture configuration shared by all capture backends.
struct CaptureConfig {
    int sample_rate = 44100;
    int channels    = 2;
};
