#include "core/CrashRecoveryCoordinator.h"
#include "core/SessionFile.h"
#include "core/UrlInput.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace vigilio {
namespace {

std::string trimStr(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool parseJsonSentinel(const std::string& raw, CrashSentinel& out) {
    std::string s = trimStr(raw);
    if (s.size() < 2 || s.front() != '{' || s.back() != '}') {
        return false;
    }

    std::size_t pos = 1;
    const std::size_t end = s.size() - 1;

    bool foundSchemaVersion = false;
    bool foundCleanExit = false;
    bool foundConsecutiveCrashes = false;

    auto skipWhitespace = [&]() {
        while (pos < end && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n')) {
            ++pos;
        }
    };

    while (pos < end) {
        skipWhitespace();
        if (pos >= end) {
            break;
        }

        if (s[pos] != '"') {
            return false;
        }
        ++pos;
        std::size_t keyStart = pos;
        while (pos < end && s[pos] != '"') {
            ++pos;
        }
        if (pos >= end || s[pos] != '"') {
            return false;
        }
        std::string key = s.substr(keyStart, pos - keyStart);
        ++pos; // skip closing quote

        skipWhitespace();
        if (pos >= end || s[pos] != ':') {
            return false;
        }
        ++pos; // skip ':'
        skipWhitespace();
        if (pos >= end) {
            return false;
        }

        if (s[pos] == '"') {
            ++pos;
            std::size_t valStart = pos;
            while (pos < end && s[pos] != '"') {
                ++pos;
            }
            if (pos >= end || s[pos] != '"') {
                return false;
            }
            std::string val = s.substr(valStart, pos - valStart);
            ++pos; // skip closing quote
            if (key == "app_version") {
                out.appVersion = val;
            }
        } else if (s.compare(pos, 4, "true") == 0) {
            pos += 4;
            if (key == "clean_exit") {
                out.cleanExit = true;
                foundCleanExit = true;
            }
        } else if (s.compare(pos, 5, "false") == 0) {
            pos += 5;
            if (key == "clean_exit") {
                out.cleanExit = false;
                foundCleanExit = true;
            }
        } else if (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '-') {
            std::size_t valStart = pos;
            if (s[pos] == '-') {
                ++pos;
            }
            while (pos < end && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                ++pos;
            }
            std::string numStr = s.substr(valStart, pos - valStart);
            try {
                int64_t num = std::stoll(numStr);
                if (key == "schema_version") {
                    if (num != 1) {
                        return false;
                    }
                    out.schemaVersion = static_cast<uint32_t>(num);
                    foundSchemaVersion = true;
                } else if (key == "consecutive_crashes") {
                    if (num < 0 || num > UINT32_MAX) {
                        return false;
                    }
                    out.consecutiveCrashes = static_cast<uint32_t>(num);
                    foundConsecutiveCrashes = true;
                } else if (key == "pid") {
                    if (num < 0) {
                        return false;
                    }
                    out.pid = static_cast<uint64_t>(num);
                } else if (key == "session_start_time") {
                    out.sessionStartTime = num;
                } else if (key == "last_heartbeat_time") {
                    out.lastHeartbeatTime = num;
                } else if (key == "last_crash_reason") {
                    out.lastCrashReason = static_cast<CrashReason>(num);
                }
            } catch (...) {
                return false;
            }
        } else {
            return false;
        }

        skipWhitespace();
        if (pos < end) {
            if (s[pos] == ',') {
                ++pos;
            } else if (pos == end) {
                break;
            } else {
                return false;
            }
        }
    }

    return foundSchemaVersion && foundCleanExit && foundConsecutiveCrashes;
}

bool readSentinel(const std::filesystem::path& path, CrashSentinel& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parseJsonSentinel(content, out);
}

static std::mutex s_sentinelIoMutex;

bool writeSentinelAtomic(const std::filesystem::path& path, const CrashSentinel& sentinel) {
    if (path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(s_sentinelIoMutex);
    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return false;
        }
    }
    const auto temp = path.string() + ".tmp";
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        file << "{\n"
             << "  \"schema_version\": " << sentinel.schemaVersion << ",\n"
             << "  \"clean_exit\": " << (sentinel.cleanExit ? "true" : "false") << ",\n"
             << "  \"consecutive_crashes\": " << sentinel.consecutiveCrashes << ",\n"
             << "  \"pid\": " << sentinel.pid << ",\n"
             << "  \"session_start_time\": " << sentinel.sessionStartTime << ",\n"
             << "  \"last_heartbeat_time\": " << sentinel.lastHeartbeatTime << ",\n"
             << "  \"last_crash_reason\": " << static_cast<int>(sentinel.lastCrashReason) << ",\n"
             << "  \"app_version\": \"" << sentinel.appVersion << "\"\n"
             << "}\n";
        file.flush();
        if (!file) {
            file.close();
            std::filesystem::remove(temp, ec);
            return false;
        }
    }
#ifdef _WIN32
    if (!MoveFileExW(std::filesystem::path(temp).c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temp, ec);
        return false;
    }
#else
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
        return false;
    }
#endif
    return true;
}

} // namespace

CrashRecoveryReport
CrashRecoveryCoordinator::inspectSessionState(const CrashRecoveryConfig& config) noexcept {
    CrashRecoveryReport report;

    // 1. Incognito Invariant (Decisión Q-001)
    if (config.isIncognito) {
        report.state = CrashDetectionState::NoPreviousSession;
        report.recommendedAction = RecoveryAction::StartClean;
        report.consecutiveCrashCount = 0;
        report.recoverableUrls = {};
        report.isSafeModeAdvised = false;
        report.sessionCorrupted = false;
        report.backupCreated = false;
        report.diagnosticMessage = "Incognito mode active: zero disk I/O";
        return report;
    }

    std::filesystem::path sentinelPath = config.sentinelFilePath;
    if (sentinelPath.empty() && !config.profileDirectory.empty()) {
        sentinelPath = std::filesystem::path(config.profileDirectory) / "sentinel.json";
    }

    std::filesystem::path sessionPath = config.sessionFilePath;
    if (sessionPath.empty() && !config.profileDirectory.empty()) {
        sessionPath = std::filesystem::path(config.profileDirectory) / "session.json";
    }

    std::error_code ec;

    // Helper to inspect and load session.json if present
    auto loadRecoverableSession = [&]() {
        if (sessionPath.empty() || !std::filesystem::exists(sessionPath, ec) || ec) {
            return;
        }
        std::ifstream file(sessionPath, std::ios::binary);
        if (!file) {
            return;
        }
        std::string header;
        if (!std::getline(file, header) || header != "VIGILIO_SESSION_V1") {
            file.close();
            report.sessionCorrupted = true;
            report.backupCreated = archiveCorruptedSession(sessionPath);
            report.state = CrashDetectionState::CorruptedSession;
            report.recommendedAction = RecoveryAction::DiscardCorrupted;
            report.failureReason = "Invalid session header";
            report.diagnosticMessage = "Session file corrupted: archived and starting clean";
            report.recoverableUrls.clear();
            return;
        }
        file.close();
        auto rawUrls = loadSession(sessionPath);
        report.recoverableUrls = sanitizeRecoverableUrls(rawUrls);
    };

    // 2. Check sentinel file existence
    if (sentinelPath.empty() || !std::filesystem::exists(sentinelPath, ec) || ec) {
        // No sentinel file exists. Check if a session file exists (migration case)
        if (!sessionPath.empty() && std::filesystem::exists(sessionPath, ec) && !ec) {
            loadRecoverableSession();
            if (report.sessionCorrupted) {
                return report;
            }
            if (!report.recoverableUrls.empty()) {
                report.state = CrashDetectionState::CleanExit;
                report.recommendedAction = RecoveryAction::RestoreSession;
                report.consecutiveCrashCount = 0;
                report.diagnosticMessage =
                    "Previous session found without sentinel: treated as clean exit";
                return report;
            }
        }
        report.state = CrashDetectionState::NoPreviousSession;
        report.recommendedAction = RecoveryAction::StartClean;
        report.consecutiveCrashCount = 0;
        report.diagnosticMessage = "No previous session sentinel found: clean startup";
        return report;
    }

    // 3. Sentinel file exists: parse it
    CrashSentinel sentinel;
    if (!readSentinel(sentinelPath, sentinel)) {
        // Corrupted sentinel!
        report.backupCreated = archiveCorruptedSession(sentinelPath);
        report.sessionCorrupted = true;
        report.state = CrashDetectionState::AbnormalTermination;
        report.recommendedAction = RecoveryAction::PromptRestore;
        report.consecutiveCrashCount = 1;
        report.failureReason = "Sentinel file corrupted or unparseable";
        report.diagnosticMessage =
            "Sentinel file corrupted: archived and treated as abnormal termination";
        loadRecoverableSession();
        return report;
    }

    // 4. Sentinel is valid: check session file
    loadRecoverableSession();
    if (report.sessionCorrupted) {
        return report;
    }

    // 5. Evaluate cleanExit
    if (sentinel.cleanExit) {
        report.state = CrashDetectionState::CleanExit;
        report.consecutiveCrashCount = 0;
        report.isSafeModeAdvised = false;
        report.recommendedAction = RecoveryAction::RestoreSession;
        report.diagnosticMessage = "Previous session exited cleanly";
    } else {
        // Abnormal termination detected
        const uint32_t currentCrashes =
            (sentinel.consecutiveCrashes == UINT32_MAX) ? UINT32_MAX : sentinel.consecutiveCrashes + 1;
        report.consecutiveCrashCount = currentCrashes;
        const uint32_t threshold = config.crashLoopThreshold > 0 ? config.crashLoopThreshold : 2;
        if (currentCrashes >= threshold) {
            report.state = CrashDetectionState::RepeatedCrashes;
            report.recommendedAction = RecoveryAction::SafeModeRecovery;
            report.isSafeModeAdvised = true;
            report.diagnosticMessage = "Repeated crashes detected: safe mode recovery advised";
        } else {
            report.state = CrashDetectionState::AbnormalTermination;
            report.recommendedAction = RecoveryAction::PromptRestore;
            report.isSafeModeAdvised = false;
            report.diagnosticMessage =
                "Abnormal session termination detected: prompt restore recommended";
        }
    }

    return report;
}

bool CrashRecoveryCoordinator::markSessionActive(const CrashRecoveryConfig& config, uint64_t pid,
                                                 int64_t timestamp) noexcept {
    if (config.isIncognito) {
        return true;
    }
    std::filesystem::path sentinelPath = config.sentinelFilePath;
    if (sentinelPath.empty() && !config.profileDirectory.empty()) {
        sentinelPath = std::filesystem::path(config.profileDirectory) / "sentinel.json";
    }
    if (sentinelPath.empty()) {
        return false;
    }

    CrashSentinel sentinel;
    std::error_code ec;
    if (std::filesystem::exists(sentinelPath, ec) && !ec) {
        CrashSentinel existing;
        if (readSentinel(sentinelPath, existing)) {
            if (!existing.cleanExit) {
                sentinel.consecutiveCrashes = (existing.consecutiveCrashes == UINT32_MAX)
                                                  ? UINT32_MAX
                                                  : existing.consecutiveCrashes + 1;
            } else {
                sentinel.consecutiveCrashes = 0;
            }
            sentinel.appVersion = existing.appVersion;
        }
    }

    sentinel.schemaVersion = 1;
    sentinel.cleanExit = false;
    sentinel.pid = pid;
    sentinel.sessionStartTime = timestamp;
    sentinel.lastHeartbeatTime = timestamp;
    sentinel.lastCrashReason = CrashReason::Unknown;

    return writeSentinelAtomic(sentinelPath, sentinel);
}

bool CrashRecoveryCoordinator::markSessionClean(const CrashRecoveryConfig& config) noexcept {
    if (config.isIncognito) {
        return true;
    }
    std::filesystem::path sentinelPath = config.sentinelFilePath;
    if (sentinelPath.empty() && !config.profileDirectory.empty()) {
        sentinelPath = std::filesystem::path(config.profileDirectory) / "sentinel.json";
    }
    if (sentinelPath.empty()) {
        return false;
    }

    CrashSentinel sentinel;
    std::error_code ec;
    if (std::filesystem::exists(sentinelPath, ec) && !ec) {
        CrashSentinel existing;
        if (readSentinel(sentinelPath, existing)) {
            sentinel = existing;
        }
    }

    sentinel.schemaVersion = 1;
    sentinel.cleanExit = true;
    sentinel.consecutiveCrashes = 0;
    sentinel.lastCrashReason = CrashReason::CleanShutdown;

    return writeSentinelAtomic(sentinelPath, sentinel);
}

bool CrashRecoveryCoordinator::updateHeartbeat(const CrashRecoveryConfig& config,
                                               int64_t timestamp) noexcept {
    if (config.isIncognito) {
        return true;
    }
    std::filesystem::path sentinelPath = config.sentinelFilePath;
    if (sentinelPath.empty() && !config.profileDirectory.empty()) {
        sentinelPath = std::filesystem::path(config.profileDirectory) / "sentinel.json";
    }
    if (sentinelPath.empty()) {
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(sentinelPath, ec) || ec) {
        return false;
    }

    CrashSentinel sentinel;
    if (!readSentinel(sentinelPath, sentinel)) {
        return false;
    }

    sentinel.lastHeartbeatTime = timestamp;
    return writeSentinelAtomic(sentinelPath, sentinel);
}

std::vector<std::string>
CrashRecoveryCoordinator::sanitizeRecoverableUrls(const std::vector<std::string>& rawUrls) noexcept {
    constexpr std::size_t kMaxTabs = 1000;
    constexpr std::size_t kMaxUrlLength = 8192;

    std::vector<std::string> sanitized;
    sanitized.reserve(std::min(rawUrls.size(), kMaxTabs));

    for (const auto& raw : rawUrls) {
        if (sanitized.size() >= kMaxTabs) {
            break;
        }
        if (raw.empty() || raw.size() > kMaxUrlLength) {
            continue;
        }
        if (raw.find_first_of("\r\n") != std::string::npos || raw.find('\0') != std::string::npos) {
            continue;
        }

        if (raw.starts_with("vigilio://")) {
            if (sanitized.empty() || sanitized.back() != raw) {
                sanitized.push_back(raw);
            }
            continue;
        }

        const auto target = resolveInput(raw);
        if (target.kind == NavigationTarget::Kind::Url && target.value == raw) {
            if (raw.starts_with("http://") || raw.starts_with("https://")) {
                if (sanitized.empty() || sanitized.back() != raw) {
                    sanitized.push_back(raw);
                }
            }
        }
    }

    return sanitized;
}

bool CrashRecoveryCoordinator::archiveCorruptedSession(
    const std::filesystem::path& sessionPath) noexcept {
    if (sessionPath.empty()) {
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::exists(sessionPath, ec) || ec) {
        return false;
    }
    const auto backupPath = sessionPath.string() + ".corrupt.bak";
    if (std::filesystem::exists(backupPath, ec)) {
        std::filesystem::remove(backupPath, ec);
    }
#ifdef _WIN32
    if (!MoveFileExW(sessionPath.c_str(), std::filesystem::path(backupPath).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return false;
    }
#else
    std::filesystem::rename(sessionPath, backupPath, ec);
    if (ec) {
        return false;
    }
#endif
    return true;
}

bool CrashRecoveryCoordinator::archiveDiscardedSession(
    const std::filesystem::path& sessionPath) noexcept {
    if (sessionPath.empty()) {
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::exists(sessionPath, ec) || ec) {
        return false;
    }
    const auto backupPath = sessionPath.string() + ".discarded.bak";
    if (std::filesystem::exists(backupPath, ec)) {
        std::filesystem::remove(backupPath, ec);
    }
#ifdef _WIN32
    if (!MoveFileExW(sessionPath.c_str(), std::filesystem::path(backupPath).c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return false;
    }
#else
    std::filesystem::rename(sessionPath, backupPath, ec);
    if (ec) {
        return false;
    }
#endif
    return true;
}

CrashRecoveryService::CrashRecoveryService(CrashRecoveryConfig config) noexcept
    : config_(std::move(config)) {
}

RecoveryServiceState CrashRecoveryService::currentState() const noexcept {
    return state_;
}

const CrashRecoveryConfig& CrashRecoveryService::configuration() const noexcept {
    return config_;
}

const CrashRecoveryReport& CrashRecoveryService::lastReport() const noexcept {
    return lastReport_;
}

const std::vector<std::string>& CrashRecoveryService::recoverableUrls() const noexcept {
    return retainedUrls_;
}

bool CrashRecoveryService::hasRecoverableUrls() const noexcept {
    return !retainedUrls_.empty();
}

CrashRecoveryReport CrashRecoveryService::initialize(uint64_t pid, int64_t timestamp) noexcept {
    return initialize(config_, pid, timestamp);
}

CrashRecoveryReport CrashRecoveryService::initialize(const CrashRecoveryConfig& config, uint64_t pid,
                                                     int64_t timestamp) noexcept {
    config_ = config;
    currentPid_ = pid;
    sessionStartTimestamp_ = timestamp;

    // 1. Incognito Invariant (Decisión Q-001)
    if (config_.isIncognito) {
        lastReport_ = CrashRecoveryCoordinator::inspectSessionState(config_);
        retainedUrls_.clear();
        state_ = RecoveryServiceState::SessionActive;
        return lastReport_;
    }

    lastReport_ = CrashRecoveryCoordinator::inspectSessionState(config_);
    retainedUrls_ = lastReport_.recoverableUrls;

    switch (lastReport_.state) {
    case CrashDetectionState::CleanExit:
    case CrashDetectionState::NoPreviousSession:
    case CrashDetectionState::Recovered: {
        const bool activeOk = CrashRecoveryCoordinator::markSessionActive(config_, pid, timestamp);
        if (activeOk) {
            state_ = RecoveryServiceState::SessionActive;
        }
        break;
    }

    case CrashDetectionState::CorruptedSession: {
        // Corrupted session was already archived by inspectSessionState
        const bool activeOk = CrashRecoveryCoordinator::markSessionActive(config_, pid, timestamp);
        if (activeOk) {
            state_ = RecoveryServiceState::SessionActive;
        }
        break;
    }

    case CrashDetectionState::AbnormalTermination:
    case CrashDetectionState::RepeatedCrashes:
        // Abnormal termination: retain URLs in memory and await user/policy decision
        state_ = RecoveryServiceState::AwaitingDecision;
        break;
    }

    return lastReport_;
}

std::vector<std::string> CrashRecoveryService::applyRecovery() noexcept {
    if (config_.isIncognito) {
        state_ = RecoveryServiceState::SessionActive;
        return {};
    }

    if (state_ == RecoveryServiceState::AwaitingDecision || state_ == RecoveryServiceState::Inspected) {
        const bool activeOk =
            CrashRecoveryCoordinator::markSessionActive(config_, currentPid_, sessionStartTimestamp_);
        if (activeOk) {
            state_ = RecoveryServiceState::SessionActive;
        }
    }

    return retainedUrls_;
}

bool CrashRecoveryService::discardRecovery() noexcept {
    if (config_.isIncognito) {
        retainedUrls_.clear();
        state_ = RecoveryServiceState::SessionActive;
        return true;
    }

    std::filesystem::path sessionPath = config_.sessionFilePath;
    if (sessionPath.empty() && !config_.profileDirectory.empty()) {
        sessionPath = std::filesystem::path(config_.profileDirectory) / "session.json";
    }

    std::error_code ec;
    if (!sessionPath.empty() && std::filesystem::exists(sessionPath, ec) && !ec) {
        const bool archived = CrashRecoveryCoordinator::archiveDiscardedSession(sessionPath);
        if (!archived) {
            // Retrying or proceeding gracefully
        }
    }

    retainedUrls_.clear();
    const bool activeOk =
        CrashRecoveryCoordinator::markSessionActive(config_, currentPid_, sessionStartTimestamp_);
    if (activeOk) {
        state_ = RecoveryServiceState::SessionActive;
    }
    return true;
}

bool CrashRecoveryService::cancelRecovery() noexcept {
    if (state_ == RecoveryServiceState::AwaitingDecision) {
        state_ = RecoveryServiceState::Inspected;
        return true;
    }
    if (state_ == RecoveryServiceState::Inspected) {
        return true;
    }
    return false;
}

bool CrashRecoveryService::recordHeartbeat(int64_t timestamp) noexcept {
    if (config_.isIncognito) {
        return true;
    }
    return CrashRecoveryCoordinator::updateHeartbeat(config_, timestamp);
}

bool CrashRecoveryService::finalizeShutdown(bool cleanExit, CrashReason /*reason*/) noexcept {
    if (config_.isIncognito) {
        state_ = RecoveryServiceState::SessionClean;
        return true;
    }

    if (cleanExit) {
        const bool ok = CrashRecoveryCoordinator::markSessionClean(config_);
        if (ok) {
            state_ = RecoveryServiceState::SessionClean;
        }
        return ok;
    }

    // Abnormal shutdown: sentinel remains clean_exit = false
    state_ = RecoveryServiceState::SessionActive;
    return true;
}

bool CrashRecoveryService::finalizeShutdown(const LifetimeReport& report) noexcept {
    return finalizeShutdown(report.isCleanExit(), report.isCleanExit() ? CrashReason::CleanShutdown
                                                                       : CrashReason::ProcessCrash);
}

} // namespace vigilio
