// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include "capture_config.h"

class Display;

// Captures audio from the default PulseAudio sink monitor
// (i.e. whatever is playing through the speakers) and feeds
// the raw PCM data into Display::set_pcm_data().
//
// Lifecycle: construct -> start() -> [audio flows] -> stop() / destructor.
// start() and stop() are idempotent and safe to call from any thread.
class PulseAudioCapture {
public:
    explicit PulseAudioCapture(Display& display, const CaptureConfig& config = {});
    ~PulseAudioCapture();

    bool start();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
