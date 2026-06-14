// Copyright (c) 2026 Duilio Protti
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include "capture_config.h"

class Display;

class PipeWireCapture {
public:
    explicit PipeWireCapture(Display& display, const CaptureConfig& config = {});
    ~PipeWireCapture();

    bool start();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
