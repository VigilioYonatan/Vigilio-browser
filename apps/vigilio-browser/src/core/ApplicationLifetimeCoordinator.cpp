#include "core/ApplicationLifetimeCoordinator.h"

#include <chrono>
#include <utility>

namespace vigilio {

ApplicationLifetimeCoordinator::ApplicationLifetimeCoordinator(LifetimeConfig config) noexcept
    : config_(std::move(config)), state_(AppLifetimeState::Uninitialized), shutdownInProgress_(false),
      shutdownRequestCount_(0) {
}

AppLifetimeState ApplicationLifetimeCoordinator::currentState() const noexcept {
    return state_.load(std::memory_order_acquire);
}

bool ApplicationLifetimeCoordinator::isShutdownRequested() const noexcept {
    const auto st = currentState();
    return st == AppLifetimeState::ShutdownRequested || st == AppLifetimeState::FlushingState ||
           st == AppLifetimeState::TerminatingSubsystems || st == AppLifetimeState::EmergencyAbort ||
           st == AppLifetimeState::Terminated;
}

bool ApplicationLifetimeCoordinator::isIncognito() const noexcept {
    return config_.isIncognito;
}

const LifetimeConfig& ApplicationLifetimeCoordinator::configuration() const noexcept {
    return config_;
}

bool ApplicationLifetimeCoordinator::transitionTo(AppLifetimeState newState) noexcept {
    auto expected = state_.load(std::memory_order_relaxed);
    while (canTransition(expected, newState)) {
        if (state_.compare_exchange_weak(expected, newState, std::memory_order_acq_rel,
                                         std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

ShutdownAction ApplicationLifetimeCoordinator::requestShutdown(ShutdownReason reason) noexcept {
    const uint32_t count = shutdownRequestCount_.fetch_add(1, std::memory_order_relaxed);

    if (shutdownInProgress_.load(std::memory_order_acquire)) {
        if (count >= 2 &&
            (reason == ShutdownReason::SignalInterrupt || reason == ShutdownReason::SignalTerminate)) {
            transitionTo(AppLifetimeState::EmergencyAbort);
            return ShutdownAction::ProceedImmediate;
        }
        return ShutdownAction::IgnoreRequest;
    }

    if (reason == ShutdownReason::EmergencyAbort || reason == ShutdownReason::MemoryExhaustion) {
        transitionTo(AppLifetimeState::EmergencyAbort);
        return ShutdownAction::ProceedImmediate;
    }

    if (transitionTo(AppLifetimeState::ShutdownRequested)) {
        return ShutdownAction::ProceedGraceful;
    }

    if (isShutdownRequested()) {
        return ShutdownAction::IgnoreRequest;
    }

    return ShutdownAction::IgnoreRequest;
}

LifetimeReport ApplicationLifetimeCoordinator::executeShutdown(ShutdownReason reason,
                                                               std::function<bool()> sessionSaver,
                                                               std::function<bool()> dbCloser) noexcept {
    LifetimeReport report;
    report.reason = reason;

    bool expectedProgress = false;
    if (!shutdownInProgress_.compare_exchange_strong(expectedProgress, true,
                                                     std::memory_order_acq_rel)) {
        report.finalState = currentState();
        report.exitCode = 0;
        report.diagnosticMessage = "Shutdown already in progress";
        return report;
    }

    if (currentState() != AppLifetimeState::ShutdownRequested &&
        currentState() != AppLifetimeState::EmergencyAbort &&
        currentState() != AppLifetimeState::FlushingState) {
        if (reason == ShutdownReason::EmergencyAbort || reason == ShutdownReason::MemoryExhaustion) {
            transitionTo(AppLifetimeState::EmergencyAbort);
        } else {
            transitionTo(AppLifetimeState::ShutdownRequested);
        }
    }

    if (currentState() == AppLifetimeState::EmergencyAbort) {
        report.sessionSaved = false;
        if (dbCloser) {
            report.databaseClosedCleanly = dbCloser();
        }
        transitionTo(AppLifetimeState::Terminated);
        report.finalState = currentState();
        if (reason == ShutdownReason::SignalInterrupt) {
            report.exitCode = 130;
        } else if (reason == ShutdownReason::SignalTerminate) {
            report.exitCode = 143;
        } else {
            report.exitCode = 1;
        }
        report.diagnosticMessage = "Emergency abort executed without flushing state";
        return report;
    }

    transitionTo(AppLifetimeState::FlushingState);

    const auto flushStart = std::chrono::steady_clock::now();

    if (config_.isIncognito) {
        report.sessionSaved = false;
    } else if (sessionSaver) {
        report.sessionSaved = sessionSaver();
    }

    const auto flushEnd = std::chrono::steady_clock::now();
    report.sessionFlushElapsedMicroseconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(flushEnd - flushStart).count());

    const auto flushElapsedMs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(flushEnd - flushStart).count());

    if (flushElapsedMs > config_.shutdownTimeoutMs ||
        currentState() == AppLifetimeState::EmergencyAbort) {
        if (flushElapsedMs > config_.shutdownTimeoutMs) {
            report.timeoutTriggered = true;
        }
        transitionTo(AppLifetimeState::EmergencyAbort);
        if (dbCloser) {
            report.databaseClosedCleanly = dbCloser();
        }
        transitionTo(AppLifetimeState::Terminated);
        report.finalState = currentState();
        report.exitCode = (reason == ShutdownReason::SignalInterrupt)
                              ? 130
                              : ((reason == ShutdownReason::SignalTerminate) ? 143 : 1);
        report.diagnosticMessage = report.timeoutTriggered
                                       ? "Shutdown timeout exceeded in FlushingState"
                                       : "Emergency abort triggered during FlushingState";
        return report;
    }

    transitionTo(AppLifetimeState::TerminatingSubsystems);

    const auto teardownStart = std::chrono::steady_clock::now();

    if (dbCloser) {
        report.databaseClosedCleanly = dbCloser();
    }

    const auto teardownEnd = std::chrono::steady_clock::now();
    report.teardownElapsedMicroseconds = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(teardownEnd - teardownStart).count());

    if (currentState() == AppLifetimeState::EmergencyAbort) {
        transitionTo(AppLifetimeState::Terminated);
        report.finalState = currentState();
        report.exitCode = 1;
        report.diagnosticMessage = "Emergency abort during TerminatingSubsystems";
        return report;
    }

    transitionTo(AppLifetimeState::Terminated);
    report.finalState = currentState();
    report.exitCode = 0;
    report.diagnosticMessage = "Graceful shutdown completed successfully";
    return report;
}

bool ApplicationLifetimeCoordinator::canTransition(AppLifetimeState from, AppLifetimeState to) noexcept {
    switch (from) {
    case AppLifetimeState::Uninitialized:
        return to == AppLifetimeState::Starting;
    case AppLifetimeState::Starting:
        return to == AppLifetimeState::Running || to == AppLifetimeState::ShutdownRequested ||
               to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::Running:
        return to == AppLifetimeState::Suspending || to == AppLifetimeState::ShutdownRequested ||
               to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::Suspending:
        return to == AppLifetimeState::Running || to == AppLifetimeState::ShutdownRequested ||
               to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::ShutdownRequested:
        return to == AppLifetimeState::FlushingState || to == AppLifetimeState::Running ||
               to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::FlushingState:
        return to == AppLifetimeState::TerminatingSubsystems || to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::TerminatingSubsystems:
        return to == AppLifetimeState::Terminated || to == AppLifetimeState::EmergencyAbort;
    case AppLifetimeState::EmergencyAbort:
        return to == AppLifetimeState::Terminated;
    case AppLifetimeState::Terminated:
        return false;
    }
    return false;
}

bool ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason reason) noexcept {
    return (reason == ShutdownReason::SignalInterrupt || reason == ShutdownReason::SignalTerminate);
}

std::string_view ApplicationLifetimeCoordinator::stateToString(AppLifetimeState state) noexcept {
    switch (state) {
    case AppLifetimeState::Uninitialized:
        return "Uninitialized";
    case AppLifetimeState::Starting:
        return "Starting";
    case AppLifetimeState::Running:
        return "Running";
    case AppLifetimeState::Suspending:
        return "Suspending";
    case AppLifetimeState::ShutdownRequested:
        return "ShutdownRequested";
    case AppLifetimeState::FlushingState:
        return "FlushingState";
    case AppLifetimeState::TerminatingSubsystems:
        return "TerminatingSubsystems";
    case AppLifetimeState::Terminated:
        return "Terminated";
    case AppLifetimeState::EmergencyAbort:
        return "EmergencyAbort";
    }
    return "Unknown";
}

std::string_view ApplicationLifetimeCoordinator::reasonToString(ShutdownReason reason) noexcept {
    switch (reason) {
    case ShutdownReason::UserWindowClose:
        return "UserWindowClose";
    case ShutdownReason::UserExplicitQuit:
        return "UserExplicitQuit";
    case ShutdownReason::SignalInterrupt:
        return "SignalInterrupt";
    case ShutdownReason::SignalTerminate:
        return "SignalTerminate";
    case ShutdownReason::SessionLogout:
        return "SessionLogout";
    case ShutdownReason::StorageFailure:
        return "StorageFailure";
    case ShutdownReason::MemoryExhaustion:
        return "MemoryExhaustion";
    case ShutdownReason::EmergencyAbort:
        return "EmergencyAbort";
    }
    return "Unknown";
}

std::string_view ApplicationLifetimeCoordinator::actionToString(ShutdownAction action) noexcept {
    switch (action) {
    case ShutdownAction::ProceedGraceful:
        return "ProceedGraceful";
    case ShutdownAction::ProceedImmediate:
        return "ProceedImmediate";
    case ShutdownAction::PromptUser:
        return "PromptUser";
    case ShutdownAction::IgnoreRequest:
        return "IgnoreRequest";
    }
    return "Unknown";
}

} // namespace vigilio
