// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Joseph Ogle

#pragma once

#include <string>
#include <string_view>

namespace aerotest {

/// Why a call refused to do the work.
/// The strings for these names are task T1. Until then, `to_string` returns "unknown".
enum class Error {
    Ok = 0,
    NotOpen,         ///< send or receive before a successful open, or after close
    Timeout,         ///< receive found no ready frame
    InvalidArgument, ///< the frame, timeout, or fault spec breaks a documented rule
    Io,              ///< the operating system rejected a SocketCAN call
    FaultInjected,   ///< an armed Error fault told the bus to fail this send
    NotImplemented   ///< the body is still a step-1 stub
};

/// Stable spelling of `error`. Task T1. The stub returns "unknown".
[[nodiscard]] const char* to_string(Error error);

/// A yes-or-no from a function that can fail, plus a human-readable detail.
/// `detail` is for logs and test output. Tests check `code`, not the wording,
/// except for `to_string`.
struct Status {
    Error code{Error::NotImplemented};
    std::string detail;

    [[nodiscard]] bool ok() const noexcept { return code == Error::Ok; }

    [[nodiscard]] static Status success() { return Status{Error::Ok, {}}; }

    [[nodiscard]] static Status failure(Error error, std::string message) {
        return Status{error, std::move(message)};
    }

    [[nodiscard]] static Status not_implemented(std::string_view what) {
        return failure(Error::NotImplemented, std::string(what));
    }
};

} // namespace aerotest
