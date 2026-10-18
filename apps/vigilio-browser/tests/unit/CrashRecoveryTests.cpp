#include "core/CrashRecoveryCoordinator.h"
#include "core/SessionFile.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            throw std::runtime_error(std::string("CHECK failed at line ") + std::to_string(__LINE__) +  \
                                     ": " #condition);                                                  \
        }                                                                                               \
    } while (false)

namespace {

struct TempDirectoryFixture {
    std::filesystem::path path;

    explicit TempDirectoryFixture(const std::string& prefix) {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / (prefix + "_" + std::to_string(now));
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    ~TempDirectoryFixture() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void test_crash_recovery_no_previous_session() {
    TempDirectoryFixture fixture("vigilio_test_no_session");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);

    CHECK(report.state == CrashDetectionState::NoPreviousSession);
    CHECK(report.recommendedAction == RecoveryAction::StartClean);
    CHECK(report.consecutiveCrashCount == 0);
    CHECK(!report.hasRecoverableTabs());
    CHECK(report.recoverableUrls.empty());
    CHECK(!report.isSafeModeAdvised);
    CHECK(!report.sessionCorrupted);
    CHECK(!report.backupCreated);
}

void test_crash_recovery_clean_exit() {
    TempDirectoryFixture fixture("vigilio_test_clean_exit");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const std::vector<std::string> tabs = {"https://example.com", "https://vigilio.dev"};
    CHECK(saveSession(config.sessionFilePath, tabs));

    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 100, 1000));
    CHECK(CrashRecoveryCoordinator::markSessionClean(config));

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);

    CHECK(report.state == CrashDetectionState::CleanExit);
    CHECK(report.recommendedAction == RecoveryAction::RestoreSession);
    CHECK(report.consecutiveCrashCount == 0);
    CHECK(report.hasRecoverableTabs());
    CHECK(report.recoverableUrls.size() == 2);
    CHECK(report.recoverableUrls[0] == "https://example.com");
    CHECK(report.recoverableUrls[1] == "https://vigilio.dev");
    CHECK(!report.isSafeModeAdvised);
    CHECK(!report.sessionCorrupted);
}

void test_crash_recovery_abnormal_termination() {
    TempDirectoryFixture fixture("vigilio_test_abnormal");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const std::vector<std::string> tabs = {"https://news.ycombinator.com"};
    CHECK(saveSession(config.sessionFilePath, tabs));

    // Active session that dies without calling markSessionClean
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 101, 1000));

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);

    CHECK(report.state == CrashDetectionState::AbnormalTermination);
    CHECK(report.recommendedAction == RecoveryAction::PromptRestore);
    CHECK(report.consecutiveCrashCount == 1);
    CHECK(report.hasRecoverableTabs());
    CHECK(report.recoverableUrls.size() == 1);
    CHECK(report.recoverableUrls[0] == "https://news.ycombinator.com");
    CHECK(!report.isSafeModeAdvised);
    CHECK(!report.sessionCorrupted);
}

void test_crash_recovery_repeated_crashes_loop() {
    TempDirectoryFixture fixture("vigilio_test_loop");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();
    config.crashLoopThreshold = 2;

    const std::vector<std::string> tabs = {"https://crashy.site"};
    CHECK(saveSession(config.sessionFilePath, tabs));

    // First crash: active and aborted
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 201, 1000));

    // Inspect confirms abnormal termination (1 crash)
    const auto firstReport = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(firstReport.state == CrashDetectionState::AbnormalTermination);
    CHECK(firstReport.consecutiveCrashCount == 1);
    CHECK(!firstReport.isSafeModeAdvised);

    // Second active run (still without clean shutdown)
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 202, 2000));

    // Inspect confirms repeated crashes loop (2 crashes >= threshold 2)
    const auto secondReport = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(secondReport.state == CrashDetectionState::RepeatedCrashes);
    CHECK(secondReport.recommendedAction == RecoveryAction::SafeModeRecovery);
    CHECK(secondReport.consecutiveCrashCount == 2);
    CHECK(secondReport.isSafeModeAdvised);
    CHECK(secondReport.hasRecoverableTabs());
}

void test_crash_recovery_incognito_zero_io() {
    TempDirectoryFixture fixture("vigilio_test_incognito");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    // Create a sentinel in normal mode first
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 301, 1000));
    std::error_code ec;
    const auto initialSize = std::filesystem::file_size(config.sentinelFilePath, ec);
    CHECK(!ec);

    // Switch to incognito
    config.isIncognito = true;

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(report.state == CrashDetectionState::NoPreviousSession);
    CHECK(report.recommendedAction == RecoveryAction::StartClean);
    CHECK(report.consecutiveCrashCount == 0);
    CHECK(report.recoverableUrls.empty());

    // Mutations must return true and leave disk untouched
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 302, 2000));
    CHECK(CrashRecoveryCoordinator::markSessionClean(config));
    CHECK(CrashRecoveryCoordinator::updateHeartbeat(config, 3000));

    const auto currentSize = std::filesystem::file_size(config.sentinelFilePath, ec);
    CHECK(!ec);
    CHECK(currentSize == initialSize);
}

void test_crash_recovery_corrupted_sentinel() {
    TempDirectoryFixture fixture("vigilio_test_corrupt_sentinel");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    // Write malformed, truncated JSON to sentinel.json
    {
        std::ofstream file(config.sentinelFilePath, std::ios::binary);
        file << "{\"schema_version\": 1, \"clean_exit\": fal";
        file.flush();
    }

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);

    CHECK(report.state == CrashDetectionState::AbnormalTermination);
    CHECK(report.recommendedAction == RecoveryAction::PromptRestore);
    CHECK(report.consecutiveCrashCount == 1);
    CHECK(report.sessionCorrupted);
    CHECK(report.backupCreated);

    std::error_code ec;
    const auto backupPath = config.sentinelFilePath + ".corrupt.bak";
    CHECK(std::filesystem::exists(backupPath, ec));
}

void test_crash_recovery_corrupted_session_file() {
    TempDirectoryFixture fixture("vigilio_test_corrupt_session");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 401, 1000));

    // Write a session file with an invalid header
    {
        std::ofstream file(config.sessionFilePath, std::ios::binary);
        file << "NOT_A_VALID_VIGILIO_SESSION_HEADER\n\"https://example.com\"\n";
        file.flush();
    }

    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);

    CHECK(report.state == CrashDetectionState::CorruptedSession);
    CHECK(report.recommendedAction == RecoveryAction::DiscardCorrupted);
    CHECK(report.sessionCorrupted);
    CHECK(report.backupCreated);
    CHECK(report.recoverableUrls.empty());

    std::error_code ec;
    const auto backupPath = config.sessionFilePath + ".corrupt.bak";
    CHECK(std::filesystem::exists(backupPath, ec));
}

void test_crash_recovery_url_sanitization() {
    const std::string longUrl = "https://example.com/" + std::string(8200, 'x');

    const std::vector<std::string> rawUrls = {
        "https://vigilio.dev",
        "https://vigilio.dev", // Contiguous duplicate
        "http://news.ycombinator.com",
        "vigilio://settings",
        "javascript:alert(1)",
        "data:text/html,<b>xss</b>",
        "file:///etc/passwd",
        "",
        longUrl,
        "https://valid.org",
    };

    const auto sanitized = CrashRecoveryCoordinator::sanitizeRecoverableUrls(rawUrls);

    CHECK(sanitized.size() == 4);
    CHECK(sanitized[0] == "https://vigilio.dev");
    CHECK(sanitized[1] == "http://news.ycombinator.com");
    CHECK(sanitized[2] == "vigilio://settings");
    CHECK(sanitized[3] == "https://valid.org");
}

void test_crash_recovery_mark_active_and_clean() {
    TempDirectoryFixture fixture("vigilio_test_lifecycle");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 501, 1000000));

    // Active session should be clean_exit: false
    auto report = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(report.state == CrashDetectionState::AbnormalTermination);
    CHECK(report.consecutiveCrashCount == 1);

    // Clean shutdown resets consecutive crashes and sets clean_exit: true
    CHECK(CrashRecoveryCoordinator::markSessionClean(config));

    report = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(report.state == CrashDetectionState::CleanExit);
    CHECK(report.consecutiveCrashCount == 0);
    CHECK(report.recommendedAction == RecoveryAction::RestoreSession);
}

void test_crash_recovery_heartbeat_update() {
    TempDirectoryFixture fixture("vigilio_test_heartbeat");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 601, 50000));
    CHECK(CrashRecoveryCoordinator::updateHeartbeat(config, 80000));

    // Reading file directly to check heartbeat timestamp
    std::ifstream file(config.sentinelFilePath);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CHECK(content.find("\"last_heartbeat_time\": 80000") != std::string::npos);
    CHECK(content.find("\"clean_exit\": false") != std::string::npos);
}

void test_crash_recovery_string_conversions() {
    // CrashDetectionState conversions
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::NoPreviousSession) ==
          "NoPreviousSession");
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::CleanExit) == "CleanExit");
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::AbnormalTermination) ==
          "AbnormalTermination");
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::RepeatedCrashes) ==
          "RepeatedCrashes");
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::CorruptedSession) ==
          "CorruptedSession");
    CHECK(CrashRecoveryCoordinator::stateToString(CrashDetectionState::Recovered) == "Recovered");
    CHECK(CrashRecoveryCoordinator::stateToString(static_cast<CrashDetectionState>(99)) == "Unknown");

    // RecoveryAction conversions
    CHECK(CrashRecoveryCoordinator::actionToString(RecoveryAction::StartClean) == "StartClean");
    CHECK(CrashRecoveryCoordinator::actionToString(RecoveryAction::RestoreSession) == "RestoreSession");
    CHECK(CrashRecoveryCoordinator::actionToString(RecoveryAction::PromptRestore) == "PromptRestore");
    CHECK(CrashRecoveryCoordinator::actionToString(RecoveryAction::SafeModeRecovery) ==
          "SafeModeRecovery");
    CHECK(CrashRecoveryCoordinator::actionToString(RecoveryAction::DiscardCorrupted) ==
          "DiscardCorrupted");
    CHECK(CrashRecoveryCoordinator::actionToString(static_cast<RecoveryAction>(99)) == "Unknown");

    // CrashReason conversions
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::Unknown) == "Unknown");
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::ProcessCrash) == "ProcessCrash");
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::WatchdogTimeout) == "WatchdogTimeout");
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::MemoryExhaustion) == "MemoryExhaustion");
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::StorageFailure) == "StorageFailure");
    CHECK(CrashRecoveryCoordinator::reasonToString(CrashReason::CleanShutdown) == "CleanShutdown");
    CHECK(CrashRecoveryCoordinator::reasonToString(static_cast<CrashReason>(99)) == "Unknown");

    // RecoveryServiceState conversions
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::Uninitialized) ==
          "Uninitialized");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::Inspected) ==
          "Inspected");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::AwaitingDecision) ==
          "AwaitingDecision");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::SessionRestored) ==
          "SessionRestored");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::SessionDiscarded) ==
          "SessionDiscarded");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::SessionActive) ==
          "SessionActive");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(RecoveryServiceState::SessionClean) ==
          "SessionClean");
    CHECK(CrashRecoveryCoordinator::serviceStateToString(static_cast<RecoveryServiceState>(99)) ==
          "Unknown");

    // RecoveryDecision conversions
    CHECK(CrashRecoveryCoordinator::decisionToString(RecoveryDecision::Pending) == "Pending");
    CHECK(CrashRecoveryCoordinator::decisionToString(RecoveryDecision::Restore) == "Restore");
    CHECK(CrashRecoveryCoordinator::decisionToString(RecoveryDecision::Discard) == "Discard");
    CHECK(CrashRecoveryCoordinator::decisionToString(RecoveryDecision::Cancel) == "Cancel");
    CHECK(CrashRecoveryCoordinator::decisionToString(static_cast<RecoveryDecision>(99)) == "Unknown");
}

void test_crash_recovery_service_clean_startup() {
    TempDirectoryFixture fixture("vigilio_test_service_clean");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CrashRecoveryService service(config);
    CHECK(service.currentState() == RecoveryServiceState::Uninitialized);

    const auto report = service.initialize(1001, 50000);
    CHECK(report.state == CrashDetectionState::NoPreviousSession);
    CHECK(service.currentState() == RecoveryServiceState::SessionActive);
    CHECK(!service.hasRecoverableUrls());
    CHECK(service.recoverableUrls().empty());

    // Record heartbeat
    CHECK(service.recordHeartbeat(60000));

    // Finalize clean shutdown
    CHECK(service.finalizeShutdown(true, CrashReason::CleanShutdown));
    CHECK(service.currentState() == RecoveryServiceState::SessionClean);

    const auto verifyReport = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(verifyReport.state == CrashDetectionState::CleanExit);
}

void test_crash_recovery_service_abnormal_recovery() {
    TempDirectoryFixture fixture("vigilio_test_service_abnormal");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const std::vector<std::string> tabs = {"https://example.com/tab1", "https://example.com/tab2"};
    CHECK(saveSession(config.sessionFilePath, tabs));
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 2001, 10000));

    CrashRecoveryService service;
    const auto report = service.initialize(config, 2002, 20000);

    CHECK(report.state == CrashDetectionState::AbnormalTermination);
    CHECK(service.currentState() == RecoveryServiceState::AwaitingDecision);
    CHECK(service.hasRecoverableUrls());
    CHECK(service.recoverableUrls().size() == 2);

    const auto restored = service.applyRecovery();
    CHECK(restored.size() == 2);
    CHECK(restored[0] == "https://example.com/tab1");
    CHECK(restored[1] == "https://example.com/tab2");
    CHECK(service.currentState() == RecoveryServiceState::SessionActive);
}

void test_crash_recovery_discard() {
    TempDirectoryFixture fixture("vigilio_test_service_discard");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const std::vector<std::string> tabs = {"https://discard-me.com"};
    CHECK(saveSession(config.sessionFilePath, tabs));
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 3001, 10000));

    CrashRecoveryService service;
    service.initialize(config, 3002, 20000);
    CHECK(service.currentState() == RecoveryServiceState::AwaitingDecision);
    CHECK(service.hasRecoverableUrls());

    CHECK(service.discardRecovery());
    CHECK(service.currentState() == RecoveryServiceState::SessionActive);
    CHECK(!service.hasRecoverableUrls());
    CHECK(service.recoverableUrls().empty());

    // Verify backup created
    std::error_code ec;
    const auto discardedBackup = config.sessionFilePath + ".discarded.bak";
    CHECK(std::filesystem::exists(discardedBackup, ec));
    CHECK(!std::filesystem::exists(config.sessionFilePath, ec));
}

void test_crash_recovery_cancel() {
    TempDirectoryFixture fixture("vigilio_test_service_cancel");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    const std::vector<std::string> tabs = {"https://keep-me.com"};
    CHECK(saveSession(config.sessionFilePath, tabs));
    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 4001, 10000));

    CrashRecoveryService service;
    service.initialize(config, 4002, 20000);
    CHECK(service.currentState() == RecoveryServiceState::AwaitingDecision);

    // Cancel recovery reverts to Inspected but preserves retained URLs
    CHECK(service.cancelRecovery());
    CHECK(service.currentState() == RecoveryServiceState::Inspected);
    CHECK(service.hasRecoverableUrls());
    CHECK(service.recoverableUrls().size() == 1);

    // Can subsequently apply recovery
    const auto restored = service.applyRecovery();
    CHECK(restored.size() == 1);
    CHECK(restored[0] == "https://keep-me.com");
    CHECK(service.currentState() == RecoveryServiceState::SessionActive);
}

void test_crash_recovery_service_shutdown_coordination() {
    TempDirectoryFixture fixture("vigilio_test_service_lifetime");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    // Clean exit case
    {
        CrashRecoveryService service(config);
        service.initialize(5001, 10000);

        LifetimeReport cleanReport;
        cleanReport.finalState = AppLifetimeState::Terminated;
        cleanReport.exitCode = 0;
        cleanReport.timeoutTriggered = false;

        CHECK(service.finalizeShutdown(cleanReport));
        CHECK(service.currentState() == RecoveryServiceState::SessionClean);

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::CleanExit);
    }

    // Abnormal / Emergency abort case
    {
        CrashRecoveryService service(config);
        service.initialize(5002, 20000);

        LifetimeReport crashReport;
        crashReport.finalState = AppLifetimeState::EmergencyAbort;
        crashReport.exitCode = 1;
        crashReport.timeoutTriggered = true;

        CHECK(service.finalizeShutdown(crashReport));
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);

        // Sentinel must remain unclean!
        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
    }
}

void test_crash_recovery_service_incognito() {
    TempDirectoryFixture fixture("vigilio_test_service_incognito");
    CrashRecoveryConfig config;
    config.isIncognito = true;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CrashRecoveryService service(config);
    const auto report = service.initialize(6001, 10000);

    CHECK(report.state == CrashDetectionState::NoPreviousSession);
    CHECK(service.currentState() == RecoveryServiceState::SessionActive);
    CHECK(!service.hasRecoverableUrls());

    const auto restored = service.applyRecovery();
    CHECK(restored.empty());

    CHECK(service.discardRecovery());
    CHECK(service.recordHeartbeat(20000));
    CHECK(service.finalizeShutdown(true));
    CHECK(service.currentState() == RecoveryServiceState::SessionClean);

    std::error_code ec;
    CHECK(!std::filesystem::exists(config.sentinelFilePath, ec));
    CHECK(!std::filesystem::exists(config.sessionFilePath, ec));
}

void test_crash_recovery_corrupted_sentinel_variations() {
    // TASK-001: 0 bytes, truncated JSON, schema_version != 1, inverted types, numbers out of range
    // All must generate .corrupt.bak and report abnormal termination without aborting.

    // 1. 0 bytes (empty file)
    {
        TempDirectoryFixture fixture("vigilio_corrupt_empty");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
        CHECK(report.recommendedAction == RecoveryAction::PromptRestore);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);
        CHECK(report.consecutiveCrashCount == 1);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sentinelFilePath + ".corrupt.bak", ec));
        CHECK(!std::filesystem::exists(config.sentinelFilePath, ec));
    }

    // 2. Truncated JSON
    {
        TempDirectoryFixture fixture("vigilio_corrupt_truncated");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file << "{\"schema_version\": 1, \"clean_exit\": fal";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sentinelFilePath + ".corrupt.bak", ec));
    }

    // 3. Schema version != 1 (e.g. schema_version: 2)
    {
        TempDirectoryFixture fixture("vigilio_corrupt_schema_v2");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file << "{\n"
                 << "  \"schema_version\": 2,\n"
                 << "  \"clean_exit\": false,\n"
                 << "  \"consecutive_crashes\": 1\n"
                 << "}\n";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sentinelFilePath + ".corrupt.bak", ec));
    }

    // 4. Inverted types (string for boolean "clean_exit": "false")
    {
        TempDirectoryFixture fixture("vigilio_corrupt_types");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file << "{\n"
                 << "  \"schema_version\": 1,\n"
                 << "  \"clean_exit\": \"false\",\n"
                 << "  \"consecutive_crashes\": 1\n"
                 << "}\n";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sentinelFilePath + ".corrupt.bak", ec));
    }

    // 5. Numbers out of range (negative consecutive crashes)
    {
        TempDirectoryFixture fixture("vigilio_corrupt_negative_num");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file << "{\n"
                 << "  \"schema_version\": 1,\n"
                 << "  \"clean_exit\": false,\n"
                 << "  \"consecutive_crashes\": -5\n"
                 << "}\n";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::AbnormalTermination);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sentinelFilePath + ".corrupt.bak", ec));
    }

    // 6. Arithmetic saturation on UINT32_MAX
    {
        TempDirectoryFixture fixture("vigilio_corrupt_overflow_max");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        {
            std::ofstream file(config.sentinelFilePath, std::ios::binary);
            file << "{\n"
                 << "  \"schema_version\": 1,\n"
                 << "  \"clean_exit\": false,\n"
                 << "  \"consecutive_crashes\": 4294967295\n"
                 << "}\n";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::RepeatedCrashes);
        CHECK(report.recommendedAction == RecoveryAction::SafeModeRecovery);
        CHECK(report.consecutiveCrashCount == UINT32_MAX);
        CHECK(report.isSafeModeAdvised);
    }
}

void test_crash_recovery_corrupted_session_variations() {
    // TASK-002: session.json vacío (0 bytes), cabecera corrupta sin VIGILIO_SESSION_V1,
    // listas con más de 1000 URLs (truncado estricto a 1000 pestañas) y URLs mayores a 8192 bytes.

    // 1. 0 bytes empty session.json
    {
        TempDirectoryFixture fixture("vigilio_session_empty");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        CHECK(CrashRecoveryCoordinator::markSessionActive(config, 701, 1000));
        {
            std::ofstream file(config.sessionFilePath, std::ios::binary);
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::CorruptedSession);
        CHECK(report.recommendedAction == RecoveryAction::DiscardCorrupted);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);
        CHECK(report.recoverableUrls.empty());

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sessionFilePath + ".corrupt.bak", ec));
        CHECK(!std::filesystem::exists(config.sessionFilePath, ec));
    }

    // 2. Corrupted header
    {
        TempDirectoryFixture fixture("vigilio_session_bad_header");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        CHECK(CrashRecoveryCoordinator::markSessionActive(config, 702, 1000));
        {
            std::ofstream file(config.sessionFilePath, std::ios::binary);
            file << "BAD_HEADER_NOT_VIGILIO\n\"https://example.com\"\n";
            file.flush();
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.state == CrashDetectionState::CorruptedSession);
        CHECK(report.recommendedAction == RecoveryAction::DiscardCorrupted);
        CHECK(report.sessionCorrupted);
        CHECK(report.backupCreated);

        std::error_code ec;
        CHECK(std::filesystem::exists(config.sessionFilePath + ".corrupt.bak", ec));
    }

    // 3. More than 1000 URLs (strict truncation to 1000)
    {
        std::vector<std::string> rawUrls;
        rawUrls.reserve(1200);
        for (int i = 0; i < 1200; ++i) {
            rawUrls.push_back("https://example.com/tab/" + std::to_string(i));
        }

        // Bounded buffer sanitization truncates strictly to 1000
        const auto sanitized = CrashRecoveryCoordinator::sanitizeRecoverableUrls(rawUrls);
        CHECK(sanitized.size() == 1000);
        CHECK(sanitized.front() == "https://example.com/tab/0");
        CHECK(sanitized.back() == "https://example.com/tab/999");

        // And max allowed tabs (1000) persists and loads cleanly in session inspection
        std::vector<std::string> maxTabsUrls(rawUrls.begin(), rawUrls.begin() + 1000);
        TempDirectoryFixture fixture("vigilio_session_max_tabs");
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "session.json").string();

        CHECK(saveSession(config.sessionFilePath, maxTabsUrls));
        CHECK(CrashRecoveryCoordinator::markSessionActive(config, 703, 1000));

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.hasRecoverableTabs());
        CHECK(report.recoverableUrls.size() == 1000);
        CHECK(report.recoverableUrls.front() == "https://example.com/tab/0");
        CHECK(report.recoverableUrls.back() == "https://example.com/tab/999");
    }

    // 4. URLs > 8192 bytes
    {
        const std::string hugeUrl = "https://example.com/" + std::string(8300, 'q');
        const std::vector<std::string> tabs = {"https://valid1.com", hugeUrl, "https://valid2.com"};
        const auto sanitized = CrashRecoveryCoordinator::sanitizeRecoverableUrls(tabs);
        CHECK(sanitized.size() == 2);
        CHECK(sanitized[0] == "https://valid1.com");
        CHECK(sanitized[1] == "https://valid2.com");
    }
}

void test_crash_recovery_security_schemes() {
    // TASK-003: Rechazo sistemático de esquemas javascript:, data:, file:, blob:, about:, vbscript:,
    // URLs con caracteres de control (\0, \r, \n) y URLs vacías, preservando únicamente http://,
    // https:// y vigilio://.
    std::string nullUrl = "https://example.com/evil";
    nullUrl.push_back('\0');
    nullUrl += "extra";

    std::string nullVigilio = "vigilio://settings";
    nullVigilio.push_back('\0');
    nullVigilio += "extra";

    const std::vector<std::string> testUrls = {
        "javascript:alert(document.cookie)",
        "javascript://test/%0aalert(1)",
        "data:text/html;base64,PHNjcmlwdD5hbGVydCgxKTwvc2NyaXB0Pg==",
        "file:///etc/passwd",
        "file://C:/Windows/System32/cmd.exe",
        "blob:https://vigilio.dev/550e8400-e29b-41d4-a716-446655440000",
        "about:blank",
        "about:config",
        "vbscript:MsgBox(\"evil\")",
        "ftp://ftp.example.com/file",
        "ws://websocket.example.com",
        "https://example.com/line\nbreak",
        "https://example.com/carriage\rreturn",
        "https://example.com/crlf\r\nbreak",
        "vigilio://settings\n",
        "vigilio://bookmarks\r",
        nullUrl,
        nullVigilio,
        "",
        "   ",
        "https://valid-safe-1.com",
        "http://valid-safe-2.org/path?q=test",
        "vigilio://settings",
        "vigilio://bookmarks",
        "vigilio://history"};

    const auto sanitized = CrashRecoveryCoordinator::sanitizeRecoverableUrls(testUrls);

    CHECK(sanitized.size() == 5);
    CHECK(sanitized[0] == "https://valid-safe-1.com");
    CHECK(sanitized[1] == "http://valid-safe-2.org/path?q=test");
    CHECK(sanitized[2] == "vigilio://settings");
    CHECK(sanitized[3] == "vigilio://bookmarks");
    CHECK(sanitized[4] == "vigilio://history");
}

void test_crash_recovery_concurrency_and_reentrancy() {
    // TASK-004: Al menos 4 hilos concurrentes invocando updateHeartbeat y recordHeartbeat
    // simultáneamente sobre el mismo archivo centinela; tras join, centinela íntegro, legible y sin .tmp
    // huérfanos.
    TempDirectoryFixture fixture("vigilio_concurrency");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    CHECK(CrashRecoveryCoordinator::markSessionActive(config, 8001, 1000));
    CrashRecoveryService service(config);

    constexpr int kNumThreads = 4;
    constexpr int kIterations = 25;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&config, &service, t]() {
            for (int i = 0; i < kIterations; ++i) {
                const int64_t ts = 10000 + (t * 1000) + i;
                if ((t + i) % 2 == 0) {
                    CHECK(CrashRecoveryCoordinator::updateHeartbeat(config, ts));
                } else {
                    CHECK(service.recordHeartbeat(ts));
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // Validate final sentinel integrity
    std::ifstream file(config.sentinelFilePath, std::ios::binary);
    CHECK(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    CHECK(content.find("\"schema_version\": 1") != std::string::npos);
    CHECK(content.find("\"clean_exit\": false") != std::string::npos);
    CHECK(content.find("\"last_heartbeat_time\":") != std::string::npos);

    // Verify inspectSessionState reads it without error
    const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
    CHECK(report.state == CrashDetectionState::AbnormalTermination);
    CHECK(!report.sessionCorrupted);

    // Verify no .tmp files exist
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(fixture.path, ec)) {
        const std::string name = entry.path().filename().string();
        CHECK(name.find(".tmp") == std::string::npos);
    }
}

void test_crash_recovery_filesystem_io_errors() {
    // TASK-005: Simula directorios de solo lectura y rutas no escribibles,
    // comprobando que markSessionActive, markSessionClean y updateHeartbeat
    // retornan false de manera limpia, sin arrojar excepciones no controladas y limpiando cualquier
    // archivo .tmp.
    TempDirectoryFixture fixture("vigilio_fs_io_errors");

    // 1. Read-only directory
    const auto readOnlyDir = fixture.path / "readonly_dir";
    std::error_code ec;
    std::filesystem::create_directories(readOnlyDir, ec);
    CHECK(!ec);

    // Make read-only
    std::filesystem::permissions(readOnlyDir,
                                 std::filesystem::perms::owner_read |
                                     std::filesystem::perms::group_read |
                                     std::filesystem::perms::others_read,
                                 std::filesystem::perm_options::replace, ec);
    CHECK(!ec);

    CrashRecoveryConfig roConfig;
    roConfig.sentinelFilePath = (readOnlyDir / "sentinel.json").string();
    roConfig.sessionFilePath = (readOnlyDir / "session.json").string();

    CHECK(!CrashRecoveryCoordinator::markSessionActive(roConfig, 9001, 1000));
    CHECK(!CrashRecoveryCoordinator::markSessionClean(roConfig));
    CHECK(!CrashRecoveryCoordinator::updateHeartbeat(roConfig, 2000));

    // Restore permissions so we can inspect and clean up cleanly
    std::filesystem::permissions(readOnlyDir, std::filesystem::perms::all,
                                 std::filesystem::perm_options::replace, ec);
    CHECK(!ec);

    for (const auto& entry : std::filesystem::directory_iterator(readOnlyDir, ec)) {
        const std::string name = entry.path().filename().string();
        CHECK(name.find(".tmp") == std::string::npos);
    }

    // 2. Non-existent uncreatable directory / empty paths
    CrashRecoveryConfig invalidConfig;
    invalidConfig.sentinelFilePath = "";
    CHECK(!CrashRecoveryCoordinator::markSessionActive(invalidConfig, 9002, 1000));
    CHECK(!CrashRecoveryCoordinator::markSessionClean(invalidConfig));
    CHECK(!CrashRecoveryCoordinator::updateHeartbeat(invalidConfig, 2000));

    invalidConfig.sentinelFilePath = "/non_existent_uncreatable_root_dir/sub/sentinel.json";
    CHECK(!CrashRecoveryCoordinator::markSessionActive(invalidConfig, 9003, 1000));
    CHECK(!CrashRecoveryCoordinator::markSessionClean(invalidConfig));
    CHECK(!CrashRecoveryCoordinator::updateHeartbeat(invalidConfig, 2000));
}

void test_crash_recovery_service_state_machine_and_idempotence() {
    // TASK-006: Invocación de applyRecovery sobre servicio no inicializado,
    // llamadas sucesivas e idempotentes a discardRecovery y cancelRecovery,
    // y re-inicialización segura de un servicio activo.
    TempDirectoryFixture fixture("vigilio_state_machine");
    CrashRecoveryConfig config;
    config.sentinelFilePath = (fixture.path / "sentinel.json").string();
    config.sessionFilePath = (fixture.path / "session.json").string();

    // 1. Uninitialized service
    CrashRecoveryService uninitService;
    CHECK(uninitService.currentState() == RecoveryServiceState::Uninitialized);
    CHECK(!uninitService.hasRecoverableUrls());
    CHECK(uninitService.recoverableUrls().empty());

    // Calling applyRecovery on uninitialized does not crash or corrupt state
    const auto uninitRestored = uninitService.applyRecovery();
    CHECK(uninitRestored.empty());
    CHECK(uninitService.currentState() == RecoveryServiceState::Uninitialized);

    // Calling cancelRecovery on uninitialized returns false
    CHECK(!uninitService.cancelRecovery());
    CHECK(uninitService.currentState() == RecoveryServiceState::Uninitialized);

    // 2. Idempotent discardRecovery
    {
        const std::vector<std::string> tabs = {"https://tab-discard.com"};
        CHECK(saveSession(config.sessionFilePath, tabs));
        CHECK(CrashRecoveryCoordinator::markSessionActive(config, 9101, 1000));

        CrashRecoveryService service;
        service.initialize(config, 9102, 2000);
        CHECK(service.currentState() == RecoveryServiceState::AwaitingDecision);
        CHECK(service.hasRecoverableUrls());

        // First discard
        CHECK(service.discardRecovery());
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);
        CHECK(!service.hasRecoverableUrls());

        // Second discard (idempotent)
        CHECK(service.discardRecovery());
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);
        CHECK(!service.hasRecoverableUrls());
    }

    // 3. Idempotent cancelRecovery
    {
        const std::vector<std::string> tabs = {"https://tab-cancel.com"};
        CHECK(saveSession(config.sessionFilePath, tabs));
        CHECK(CrashRecoveryCoordinator::markSessionActive(config, 9103, 3000));

        CrashRecoveryService service;
        service.initialize(config, 9104, 4000);
        CHECK(service.currentState() == RecoveryServiceState::AwaitingDecision);

        // First cancel
        CHECK(service.cancelRecovery());
        CHECK(service.currentState() == RecoveryServiceState::Inspected);

        // Second cancel (idempotent)
        CHECK(service.cancelRecovery());
        CHECK(service.currentState() == RecoveryServiceState::Inspected);

        // Subsequent apply
        const auto restored = service.applyRecovery();
        CHECK(restored.size() == 1);
        CHECK(restored[0] == "https://tab-cancel.com");
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);
    }

    // 4. Safe re-initialization of an active service
    {
        TempDirectoryFixture activeFixture("vigilio_active_init");
        CrashRecoveryConfig activeConfig;
        activeConfig.sentinelFilePath = (activeFixture.path / "sentinel.json").string();
        activeConfig.sessionFilePath = (activeFixture.path / "session.json").string();

        CrashRecoveryService service;
        service.initialize(activeConfig, 9105, 5000);
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);

        // Re-initialize with clean config
        TempDirectoryFixture newFixture("vigilio_reinit");
        CrashRecoveryConfig newConfig;
        newConfig.sentinelFilePath = (newFixture.path / "sentinel.json").string();
        newConfig.sessionFilePath = (newFixture.path / "session.json").string();

        const auto report = service.initialize(newConfig, 9106, 6000);
        CHECK(report.state == CrashDetectionState::NoPreviousSession);
        CHECK(service.currentState() == RecoveryServiceState::SessionActive);
        CHECK(service.configuration().sentinelFilePath == newConfig.sentinelFilePath);
    }
}

void test_crash_recovery_backup_overwriting() {
    // TASK-007: Archivar sesión o centinela corrupto cuando ya existe un backup previo
    // (.corrupt.bak o .discarded.bak); se sobrescribe de forma atómica y ordenada.
    TempDirectoryFixture fixture("vigilio_backup_overwriting");
    const auto sessionPath = fixture.path / "session.json";
    const auto corruptBackupPath = fixture.path / "session.json.corrupt.bak";
    const auto discardedBackupPath = fixture.path / "session.json.discarded.bak";

    // 1. Overwrite existing .corrupt.bak
    {
        // Create pre-existing .corrupt.bak with old content
        {
            std::ofstream oldBak(corruptBackupPath, std::ios::binary);
            oldBak << "PRE_EXISTING_OLD_CORRUPT_BACKUP\n";
        }

        // Create new corrupted session file
        {
            std::ofstream newCorrupt(sessionPath, std::ios::binary);
            newCorrupt << "NEW_CORRUPTED_SESSION_CONTENT\n";
        }

        CHECK(CrashRecoveryCoordinator::archiveCorruptedSession(sessionPath));

        std::error_code ec;
        CHECK(std::filesystem::exists(corruptBackupPath, ec));
        CHECK(!std::filesystem::exists(sessionPath, ec));

        std::ifstream file(corruptBackupPath, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(content.find("NEW_CORRUPTED_SESSION_CONTENT") != std::string::npos);
        CHECK(content.find("PRE_EXISTING_OLD_CORRUPT_BACKUP") == std::string::npos);
    }

    // 2. Overwrite existing .discarded.bak
    {
        // Create pre-existing .discarded.bak with old content
        {
            std::ofstream oldBak(discardedBackupPath, std::ios::binary);
            oldBak << "PRE_EXISTING_OLD_DISCARDED_BACKUP\n";
        }

        // Create new discarded session file
        {
            std::ofstream newDiscarded(sessionPath, std::ios::binary);
            newDiscarded << "NEW_DISCARDED_SESSION_CONTENT\n";
        }

        CHECK(CrashRecoveryCoordinator::archiveDiscardedSession(sessionPath));

        std::error_code ec;
        CHECK(std::filesystem::exists(discardedBackupPath, ec));
        CHECK(!std::filesystem::exists(sessionPath, ec));

        std::ifstream file(discardedBackupPath, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(content.find("NEW_DISCARDED_SESSION_CONTENT") != std::string::npos);
        CHECK(content.find("PRE_EXISTING_OLD_DISCARDED_BACKUP") == std::string::npos);
    }

    // 3. inspectSessionState overwrites pre-existing sentinel .corrupt.bak
    {
        CrashRecoveryConfig config;
        config.sentinelFilePath = (fixture.path / "sentinel.json").string();
        config.sessionFilePath = (fixture.path / "sentinel_session.json").string();

        const auto sentinelCorruptBackup = fixture.path / "sentinel.json.corrupt.bak";
        {
            std::ofstream oldBak(sentinelCorruptBackup, std::ios::binary);
            oldBak << "OLD_SENTINEL_BACKUP\n";
        }

        // Write corrupt sentinel
        {
            std::ofstream badSentinel(config.sentinelFilePath, std::ios::binary);
            badSentinel << "BAD_SENTINEL_CONTENT_OVERWRITE\n";
        }

        const auto report = CrashRecoveryCoordinator::inspectSessionState(config);
        CHECK(report.backupCreated);

        std::ifstream file(sentinelCorruptBackup, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(content.find("BAD_SENTINEL_CONTENT_OVERWRITE") != std::string::npos);
        CHECK(content.find("OLD_SENTINEL_BACKUP") == std::string::npos);
    }
}

} // namespace

int main() {
    try {
        test_crash_recovery_no_previous_session();
        test_crash_recovery_clean_exit();
        test_crash_recovery_abnormal_termination();
        test_crash_recovery_repeated_crashes_loop();
        test_crash_recovery_incognito_zero_io();
        test_crash_recovery_corrupted_sentinel();
        test_crash_recovery_corrupted_session_file();
        test_crash_recovery_url_sanitization();
        test_crash_recovery_mark_active_and_clean();
        test_crash_recovery_heartbeat_update();
        test_crash_recovery_string_conversions();

        test_crash_recovery_service_clean_startup();
        test_crash_recovery_service_abnormal_recovery();
        test_crash_recovery_discard();
        test_crash_recovery_cancel();
        test_crash_recovery_service_shutdown_coordination();
        test_crash_recovery_service_incognito();

        test_crash_recovery_corrupted_sentinel_variations();
        test_crash_recovery_corrupted_session_variations();
        test_crash_recovery_security_schemes();
        test_crash_recovery_concurrency_and_reentrancy();
        test_crash_recovery_filesystem_io_errors();
        test_crash_recovery_service_state_machine_and_idempotence();
        test_crash_recovery_backup_overwriting();

        std::cout << "Crash recovery tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
