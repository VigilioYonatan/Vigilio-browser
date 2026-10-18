#pragma once

#include <cstdint>
#include <string>

namespace vigilio {

enum class AppLifetimeState : uint8_t {
    Uninitialized = 0,
    Starting = 1,
    Running = 2,
    Suspending = 3,
    ShutdownRequested = 4,
    FlushingState = 5,
    TerminatingSubsystems = 6,
    Terminated = 7,
    EmergencyAbort = 8
};

enum class ShutdownReason : uint8_t {
    UserWindowClose = 0,
    UserExplicitQuit = 1,
    SignalInterrupt = 2,
    SignalTerminate = 3,
    SessionLogout = 4,
    StorageFailure = 5,
    MemoryExhaustion = 6,
    EmergencyAbort = 7
};

enum class ShutdownAction : uint8_t {
    ProceedGraceful = 0,
    ProceedImmediate = 1,
    PromptUser = 2,
    IgnoreRequest = 3
};

struct LifetimeConfig {
    bool isIncognito = false;
    bool isSafeMode = false;
    uint32_t shutdownTimeoutMs = 3000;
    uint32_t sessionAutosaveIntervalMs = 0;
    std::string sessionFilePath;
    std::string profileDirectory;
};

struct LifetimeReport {
    AppLifetimeState finalState = AppLifetimeState::Uninitialized;
    ShutdownReason reason = ShutdownReason::UserWindowClose;
    int exitCode = 0;
    bool sessionSaved = false;
    bool databaseClosedCleanly = false;
    bool timeoutTriggered = false;
    uint64_t sessionFlushElapsedMicroseconds = 0;
    uint64_t teardownElapsedMicroseconds = 0;
    std::string diagnosticMessage;

    [[nodiscard]] constexpr bool isCleanExit() const noexcept {
        return (finalState == AppLifetimeState::Terminated && exitCode == 0 && !timeoutTriggered);
    }
};

} // namespace vigilio
