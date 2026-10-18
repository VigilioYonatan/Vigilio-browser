#pragma once

#include "LifetimeTypes.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <string_view>

namespace vigilio {

class ApplicationLifetimeCoordinator {
  public:
    explicit ApplicationLifetimeCoordinator(LifetimeConfig config) noexcept;

    [[nodiscard]] AppLifetimeState currentState() const noexcept;
    [[nodiscard]] bool isShutdownRequested() const noexcept;
    [[nodiscard]] bool isIncognito() const noexcept;
    [[nodiscard]] const LifetimeConfig& configuration() const noexcept;

    bool transitionTo(AppLifetimeState newState) noexcept;
    ShutdownAction requestShutdown(ShutdownReason reason) noexcept;

    LifetimeReport executeShutdown(ShutdownReason reason, std::function<bool()> sessionSaver = nullptr,
                                   std::function<bool()> dbCloser = nullptr) noexcept;

    [[nodiscard]] static bool canTransition(AppLifetimeState from, AppLifetimeState to) noexcept;
    [[nodiscard]] static bool isSignalSafe(ShutdownReason reason) noexcept;
    [[nodiscard]] static std::string_view stateToString(AppLifetimeState state) noexcept;
    [[nodiscard]] static std::string_view reasonToString(ShutdownReason reason) noexcept;
    [[nodiscard]] static std::string_view actionToString(ShutdownAction action) noexcept;

  private:
    LifetimeConfig config_;
    std::atomic<AppLifetimeState> state_{AppLifetimeState::Uninitialized};
    std::atomic<bool> shutdownInProgress_{false};
    std::atomic<uint32_t> shutdownRequestCount_{0};
};

} // namespace vigilio
