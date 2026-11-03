// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// SDK libraries may print diagnostics even when their log level is ERROR.
// Business output uses a separate handle to the original terminal.
class SdkOutput {
    FILE* capture_ = nullptr;
    FILE* console_ = nullptr;
    int out_ = -1, err_ = -1;
    void restore() noexcept {
        std::fflush(nullptr);
        if (out_ >= 0) { ::dup2(out_, STDOUT_FILENO); ::close(out_); out_ = -1; }
        if (err_ >= 0) { ::dup2(err_, STDERR_FILENO); ::close(err_); err_ = -1; }
        if (console_) { std::fclose(console_); console_ = nullptr; }
        if (capture_) { std::fclose(capture_); capture_ = nullptr; }
    }
public:
    explicit SdkOutput(const std::string& log) {
        std::cout.flush(); std::cerr.flush(); std::fflush(nullptr);
        try {
            if (log.empty()) capture_ = std::tmpfile();
            else {
                int fd = ::open(log.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0600);
                if (fd >= 0) {
                    ::fchmod(fd, 0600);
                    capture_ = ::fdopen(fd, "w+");
                    if (!capture_) ::close(fd);
                }
            }
            if (!capture_) throw std::runtime_error("Cannot open SDK diagnostic log");
            out_ = ::dup(STDOUT_FILENO); err_ = ::dup(STDERR_FILENO);
            if (out_ < 0 || err_ < 0) throw std::runtime_error("Cannot preserve terminal output");
            int fd = ::dup(out_);
            if (fd >= 0) { console_ = ::fdopen(fd, "w"); if (!console_) ::close(fd); }
            if (!console_ || ::dup2(::fileno(capture_), STDOUT_FILENO) < 0 ||
                ::dup2(::fileno(capture_), STDERR_FILENO) < 0)
                throw std::runtime_error("Cannot capture SDK diagnostics");
        } catch (...) { restore(); throw; }
    }
    ~SdkOutput() { restore(); }
    SdkOutput(const SdkOutput&) = delete;
    SdkOutput& operator=(const SdkOutput&) = delete;
    FILE* console() const { return console_; }
};
