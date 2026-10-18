#pragma once

#include "core/CrashRecoveryTypes.h"
#include "core/LifetimeTypes.h"
#include <filesystem>
#include <string_view>
#include <vector>

namespace vigilio {

class CrashRecoveryCoordinator {
  public:
    [[nodiscard]] static CrashRecoveryReport
    inspectSessionState(const CrashRecoveryConfig& config) noexcept;

    [[nodiscard]] static bool markSessionActive(const CrashRecoveryConfig& config, uint64_t pid,
                                                int64_t timestamp) noexcept;

    [[nodiscard]] static bool markSessionClean(const CrashRecoveryConfig& config) noexcept;

    [[nodiscard]] static bool updateHeartbeat(const CrashRecoveryConfig& config,
                                              int64_t timestamp) noexcept;

    [[nodiscard]] static std::vector<std::string>
    sanitizeRecoverableUrls(const std::vector<std::string>& rawUrls) noexcept;

    [[nodiscard]] static bool archiveCorruptedSession(const std::filesystem::path& sessionPath) noexcept;

    [[nodiscard]] static bool archiveDiscardedSession(const std::filesystem::path& sessionPath) noexcept;

    [[nodiscard]] static constexpr std::string_view stateToString(CrashDetectionState state) noexcept {
        switch (state) {
        case CrashDetectionState::NoPreviousSession:
            return "NoPreviousSession";
        case CrashDetectionState::CleanExit:
            return "CleanExit";
        case CrashDetectionState::AbnormalTermination:
            return "AbnormalTermination";
        case CrashDetectionState::RepeatedCrashes:
            return "RepeatedCrashes";
        case CrashDetectionState::CorruptedSession:
            return "CorruptedSession";
        case CrashDetectionState::Recovered:
            return "Recovered";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view actionToString(RecoveryAction action) noexcept {
        switch (action) {
        case RecoveryAction::StartClean:
            return "StartClean";
        case RecoveryAction::RestoreSession:
            return "RestoreSession";
        case RecoveryAction::PromptRestore:
            return "PromptRestore";
        case RecoveryAction::SafeModeRecovery:
            return "SafeModeRecovery";
        case RecoveryAction::DiscardCorrupted:
            return "DiscardCorrupted";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view reasonToString(CrashReason reason) noexcept {
        switch (reason) {
        case CrashReason::Unknown:
            return "Unknown";
        case CrashReason::ProcessCrash:
            return "ProcessCrash";
        case CrashReason::WatchdogTimeout:
            return "WatchdogTimeout";
        case CrashReason::MemoryExhaustion:
            return "MemoryExhaustion";
        case CrashReason::StorageFailure:
            return "StorageFailure";
        case CrashReason::CleanShutdown:
            return "CleanShutdown";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view
    serviceStateToString(RecoveryServiceState state) noexcept {
        switch (state) {
        case RecoveryServiceState::Uninitialized:
            return "Uninitialized";
        case RecoveryServiceState::Inspected:
            return "Inspected";
        case RecoveryServiceState::AwaitingDecision:
            return "AwaitingDecision";
        case RecoveryServiceState::SessionRestored:
            return "SessionRestored";
        case RecoveryServiceState::SessionDiscarded:
            return "SessionDiscarded";
        case RecoveryServiceState::SessionActive:
            return "SessionActive";
        case RecoveryServiceState::SessionClean:
            return "SessionClean";
        }
        return "Unknown";
    }

    [[nodiscard]] static constexpr std::string_view
    decisionToString(RecoveryDecision decision) noexcept {
        switch (decision) {
        case RecoveryDecision::Pending:
            return "Pending";
        case RecoveryDecision::Restore:
            return "Restore";
        case RecoveryDecision::Discard:
            return "Discard";
        case RecoveryDecision::Cancel:
            return "Cancel";
        }
        return "Unknown";
    }
};

class CrashRecoveryService {
  public:
    explicit CrashRecoveryService(CrashRecoveryConfig config = {}) noexcept;

    [[nodiscard]] RecoveryServiceState currentState() const noexcept;
    [[nodiscard]] const CrashRecoveryConfig& configuration() const noexcept;
    [[nodiscard]] const CrashRecoveryReport& lastReport() const noexcept;
    [[nodiscard]] const std::vector<std::string>& recoverableUrls() const noexcept;
    [[nodiscard]] bool hasRecoverableUrls() const noexcept;

    CrashRecoveryReport initialize(const CrashRecoveryConfig& config, uint64_t pid,
                                   int64_t timestamp) noexcept;
    CrashRecoveryReport initialize(uint64_t pid, int64_t timestamp) noexcept;

    [[nodiscard]] std::vector<std::string> applyRecovery() noexcept;
    [[nodiscard]] bool discardRecovery() noexcept;
    [[nodiscard]] bool cancelRecovery() noexcept;

    [[nodiscard]] bool recordHeartbeat(int64_t timestamp) noexcept;
    [[nodiscard]] bool finalizeShutdown(bool cleanExit,
                                        CrashReason reason = CrashReason::CleanShutdown) noexcept;
    [[nodiscard]] bool finalizeShutdown(const LifetimeReport& report) noexcept;

  private:
    CrashRecoveryConfig config_;
    RecoveryServiceState state_{RecoveryServiceState::Uninitialized};
    CrashRecoveryReport lastReport_{};
    std::vector<std::string> retainedUrls_{};
    uint64_t currentPid_{0};
    int64_t sessionStartTimestamp_{0};
};

} // namespace vigilio
