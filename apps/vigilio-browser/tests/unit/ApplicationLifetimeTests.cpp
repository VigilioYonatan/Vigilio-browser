#include "core/ApplicationLifetimeCoordinator.h"

#include <atomic>
#include <chrono>
#include <cstdint>
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

void test_lifetime_initial_state() {
    LifetimeConfig config;
    config.isIncognito = false;
    config.isSafeMode = false;
    config.shutdownTimeoutMs = 3000;

    ApplicationLifetimeCoordinator coordinator(config);

    CHECK(coordinator.currentState() == AppLifetimeState::Uninitialized);
    CHECK(!coordinator.isShutdownRequested());
    CHECK(!coordinator.isIncognito());
    CHECK(coordinator.configuration().shutdownTimeoutMs == 3000);

    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.currentState() == AppLifetimeState::Starting);
    CHECK(!coordinator.isShutdownRequested());

    CHECK(coordinator.transitionTo(AppLifetimeState::Running));
    CHECK(coordinator.currentState() == AppLifetimeState::Running);
    CHECK(!coordinator.isShutdownRequested());

    // Invalid backwards or jump transitions
    CHECK(!coordinator.transitionTo(AppLifetimeState::Uninitialized));
    CHECK(!coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.currentState() == AppLifetimeState::Running);
}

void test_lifetime_transition_matrix() {
    const std::vector<AppLifetimeState> states = {
        AppLifetimeState::Uninitialized,
        AppLifetimeState::Starting,
        AppLifetimeState::Running,
        AppLifetimeState::Suspending,
        AppLifetimeState::ShutdownRequested,
        AppLifetimeState::FlushingState,
        AppLifetimeState::TerminatingSubsystems,
        AppLifetimeState::Terminated,
        AppLifetimeState::EmergencyAbort,
    };

    auto isExpectedLegal = [](AppLifetimeState from, AppLifetimeState to) -> bool {
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
            return to == AppLifetimeState::TerminatingSubsystems ||
                   to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::TerminatingSubsystems:
            return to == AppLifetimeState::Terminated || to == AppLifetimeState::EmergencyAbort;
        case AppLifetimeState::EmergencyAbort:
            return to == AppLifetimeState::Terminated;
        case AppLifetimeState::Terminated:
            return false;
        }
        return false;
    };

    for (const auto from : states) {
        for (const auto to : states) {
            const bool expected = isExpectedLegal(from, to);
            const bool actual = ApplicationLifetimeCoordinator::canTransition(from, to);
            CHECK(actual == expected);
        }
    }
}

void test_lifetime_graceful_shutdown() {
    LifetimeConfig config;
    config.isIncognito = false;
    config.shutdownTimeoutMs = 3000;
    ApplicationLifetimeCoordinator coordinator(config);

    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    const auto action = coordinator.requestShutdown(ShutdownReason::UserWindowClose);
    CHECK(action == ShutdownAction::ProceedGraceful);
    CHECK(coordinator.currentState() == AppLifetimeState::ShutdownRequested);
    CHECK(coordinator.isShutdownRequested());

    bool sessionSaverCalled = false;
    bool dbCloserCalled = false;

    const auto report = coordinator.executeShutdown(
        ShutdownReason::UserWindowClose,
        [&sessionSaverCalled]() {
            sessionSaverCalled = true;
            return true;
        },
        [&dbCloserCalled]() {
            dbCloserCalled = true;
            return true;
        });

    CHECK(sessionSaverCalled);
    CHECK(dbCloserCalled);
    CHECK(report.sessionSaved);
    CHECK(report.databaseClosedCleanly);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.reason == ShutdownReason::UserWindowClose);
    CHECK(report.exitCode == 0);
    CHECK(!report.timeoutTriggered);
    CHECK(report.isCleanExit());
    CHECK(coordinator.currentState() == AppLifetimeState::Terminated);
}

void test_lifetime_incognito_zero_trace() {
    LifetimeConfig config;
    config.isIncognito = true;
    config.shutdownTimeoutMs = 3000;
    ApplicationLifetimeCoordinator coordinator(config);

    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));
    CHECK(coordinator.isIncognito());

    const auto action = coordinator.requestShutdown(ShutdownReason::UserExplicitQuit);
    CHECK(action == ShutdownAction::ProceedGraceful);

    bool sessionSaverCalled = false;
    bool dbCloserCalled = false;

    const auto report = coordinator.executeShutdown(
        ShutdownReason::UserExplicitQuit,
        [&sessionSaverCalled]() {
            sessionSaverCalled = true;
            return true;
        },
        [&dbCloserCalled]() {
            dbCloserCalled = true;
            return true;
        });

    // Invariant: Incognito never executes sessionSaver and sessionSaved is false
    CHECK(!sessionSaverCalled);
    CHECK(!report.sessionSaved);
    CHECK(dbCloserCalled);
    CHECK(report.databaseClosedCleanly);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.exitCode == 0);
    CHECK(report.isCleanExit());
}

void test_lifetime_signal_interrupt_handling() {
    CHECK(ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason::SignalInterrupt));

    LifetimeConfig config;
    ApplicationLifetimeCoordinator coordinator(config);
    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    const auto action = coordinator.requestShutdown(ShutdownReason::SignalInterrupt);
    CHECK(action == ShutdownAction::ProceedGraceful);
    CHECK(coordinator.isShutdownRequested());

    const auto report = coordinator.executeShutdown(ShutdownReason::SignalInterrupt);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.reason == ShutdownReason::SignalInterrupt);
    CHECK(report.exitCode == 0);
    CHECK(report.isCleanExit());
}

void test_lifetime_signal_terminate_handling() {
    CHECK(ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason::SignalTerminate));
    CHECK(!ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason::UserWindowClose));
    CHECK(!ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason::UserExplicitQuit));
    CHECK(!ApplicationLifetimeCoordinator::isSignalSafe(ShutdownReason::StorageFailure));

    LifetimeConfig config;
    ApplicationLifetimeCoordinator coordinator(config);
    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    const auto action = coordinator.requestShutdown(ShutdownReason::SignalTerminate);
    CHECK(action == ShutdownAction::ProceedGraceful);

    bool dbClosed = false;
    const auto report =
        coordinator.executeShutdown(ShutdownReason::SignalTerminate, nullptr, [&dbClosed]() {
            dbClosed = true;
            return true;
        });

    CHECK(dbClosed);
    CHECK(report.databaseClosedCleanly);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.reason == ShutdownReason::SignalTerminate);
    CHECK(report.exitCode == 0);
    CHECK(report.isCleanExit());
}

void test_lifetime_idempotent_shutdown_requests() {
    LifetimeConfig config;
    ApplicationLifetimeCoordinator coordinator(config);
    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    constexpr size_t threadCount = 32;
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    std::atomic<size_t> gracefulCount{0};
    std::atomic<size_t> ignoreCount{0};
    std::atomic<size_t> otherCount{0};

    for (size_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([&coordinator, &gracefulCount, &ignoreCount, &otherCount]() {
            const auto action = coordinator.requestShutdown(ShutdownReason::UserWindowClose);
            if (action == ShutdownAction::ProceedGraceful) {
                gracefulCount.fetch_add(1, std::memory_order_relaxed);
            } else if (action == ShutdownAction::IgnoreRequest) {
                ignoreCount.fetch_add(1, std::memory_order_relaxed);
            } else {
                otherCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    CHECK(gracefulCount.load() == 1);
    CHECK(ignoreCount.load() == threadCount - 1);
    CHECK(otherCount.load() == 0);
    CHECK(coordinator.currentState() == AppLifetimeState::ShutdownRequested);
}

void test_lifetime_repeated_signals_emergency() {
    LifetimeConfig config;
    ApplicationLifetimeCoordinator coordinator(config);
    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    // First request initiates shutdown
    const auto action1 = coordinator.requestShutdown(ShutdownReason::SignalInterrupt);
    CHECK(action1 == ShutdownAction::ProceedGraceful);
    CHECK(coordinator.currentState() == AppLifetimeState::ShutdownRequested);

    LifetimeReport report;
    std::thread worker([&coordinator, &report]() {
        report = coordinator.executeShutdown(
            ShutdownReason::SignalInterrupt,
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return true;
            },
            []() { return true; });
    });

    // Wait for worker to enter executeShutdown and set shutdownInProgress_
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Second signal during shutdown in progress is ignored
    const auto action2 = coordinator.requestShutdown(ShutdownReason::SignalInterrupt);
    CHECK(action2 == ShutdownAction::IgnoreRequest);

    // Third signal (>2 requests) accelerates to emergency abort
    const auto action3 = coordinator.requestShutdown(ShutdownReason::SignalInterrupt);
    CHECK(action3 == ShutdownAction::ProceedImmediate);
    CHECK(coordinator.currentState() == AppLifetimeState::EmergencyAbort);

    worker.join();

    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.exitCode == 130);
    CHECK(!report.isCleanExit());
}

void test_lifetime_timeout_watchdog() {
    LifetimeConfig config;
    config.shutdownTimeoutMs = 25; // 25ms timeout
    ApplicationLifetimeCoordinator coordinator(config);
    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));

    const auto report = coordinator.executeShutdown(
        ShutdownReason::UserWindowClose,
        []() {
            // Intentionally exceed shutdownTimeoutMs
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            return true;
        },
        []() { return true; });

    CHECK(report.timeoutTriggered);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.exitCode == 1);
    CHECK(!report.isCleanExit());
}

void test_lifetime_database_close_cleanly() {
    LifetimeConfig config;
    {
        ApplicationLifetimeCoordinator coordinator(config);
        CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
        CHECK(coordinator.transitionTo(AppLifetimeState::Running));

        const auto report =
            coordinator.executeShutdown(ShutdownReason::UserWindowClose, nullptr, []() { return true; });
        CHECK(report.databaseClosedCleanly);
    }
    {
        ApplicationLifetimeCoordinator coordinator(config);
        CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
        CHECK(coordinator.transitionTo(AppLifetimeState::Running));

        const auto report = coordinator.executeShutdown(ShutdownReason::UserWindowClose, nullptr,
                                                        []() { return false; });
        CHECK(!report.databaseClosedCleanly);
    }
}

void test_lifetime_string_conversions() {
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::Uninitialized) ==
          "Uninitialized");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::Starting) == "Starting");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::Running) == "Running");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::Suspending) == "Suspending");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::ShutdownRequested) ==
          "ShutdownRequested");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::FlushingState) ==
          "FlushingState");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::TerminatingSubsystems) ==
          "TerminatingSubsystems");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::Terminated) == "Terminated");
    CHECK(ApplicationLifetimeCoordinator::stateToString(AppLifetimeState::EmergencyAbort) ==
          "EmergencyAbort");
    CHECK(ApplicationLifetimeCoordinator::stateToString(static_cast<AppLifetimeState>(99)) == "Unknown");

    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::UserWindowClose) ==
          "UserWindowClose");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::UserExplicitQuit) ==
          "UserExplicitQuit");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::SignalInterrupt) ==
          "SignalInterrupt");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::SignalTerminate) ==
          "SignalTerminate");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::SessionLogout) ==
          "SessionLogout");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::StorageFailure) ==
          "StorageFailure");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::MemoryExhaustion) ==
          "MemoryExhaustion");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(ShutdownReason::EmergencyAbort) ==
          "EmergencyAbort");
    CHECK(ApplicationLifetimeCoordinator::reasonToString(static_cast<ShutdownReason>(99)) == "Unknown");

    CHECK(ApplicationLifetimeCoordinator::actionToString(ShutdownAction::ProceedGraceful) ==
          "ProceedGraceful");
    CHECK(ApplicationLifetimeCoordinator::actionToString(ShutdownAction::ProceedImmediate) ==
          "ProceedImmediate");
    CHECK(ApplicationLifetimeCoordinator::actionToString(ShutdownAction::PromptUser) == "PromptUser");
    CHECK(ApplicationLifetimeCoordinator::actionToString(ShutdownAction::IgnoreRequest) ==
          "IgnoreRequest");
    CHECK(ApplicationLifetimeCoordinator::actionToString(static_cast<ShutdownAction>(99)) == "Unknown");
}

void test_lifetime_cancellation_and_recovery() {
    LifetimeConfig config;
    ApplicationLifetimeCoordinator coordinator(config);

    CHECK(coordinator.transitionTo(AppLifetimeState::Starting));
    CHECK(coordinator.transitionTo(AppLifetimeState::Running));
    CHECK(!coordinator.isShutdownRequested());

    // 1. User initiates window close -> enters ShutdownRequested
    const auto action1 = coordinator.requestShutdown(ShutdownReason::UserWindowClose);
    CHECK(action1 == ShutdownAction::ProceedGraceful);
    CHECK(coordinator.currentState() == AppLifetimeState::ShutdownRequested);
    CHECK(coordinator.isShutdownRequested());

    // 2. User cancels multiple tabs close dialog -> reverts to Running
    const bool reverted = coordinator.transitionTo(AppLifetimeState::Running);
    CHECK(reverted);
    CHECK(coordinator.currentState() == AppLifetimeState::Running);
    CHECK(!coordinator.isShutdownRequested());

    // 3. System continues running and later accepts explicit quit
    const auto action2 = coordinator.requestShutdown(ShutdownReason::UserExplicitQuit);
    CHECK(action2 == ShutdownAction::ProceedGraceful);
    CHECK(coordinator.currentState() == AppLifetimeState::ShutdownRequested);
    CHECK(coordinator.isShutdownRequested());

    bool sessionSaved = false;
    bool dbClosed = false;
    const auto report = coordinator.executeShutdown(
        ShutdownReason::UserExplicitQuit,
        [&sessionSaved]() {
            sessionSaved = true;
            return true;
        },
        [&dbClosed]() {
            dbClosed = true;
            return true;
        });

    CHECK(sessionSaved);
    CHECK(dbClosed);
    CHECK(report.sessionSaved);
    CHECK(report.databaseClosedCleanly);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.exitCode == 0);
    CHECK(report.isCleanExit());
}

void test_lifetime_callback_failures_resilience() {
    LifetimeConfig config;

    // Subcase A: sessionSaver returns false (disk write failure)
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));
        coord.requestShutdown(ShutdownReason::UserWindowClose);

        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose, []() { return false; }, []() { return true; });

        CHECK(!report.sessionSaved);
        CHECK(report.databaseClosedCleanly);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }

    // Subcase B: nullptr callbacks passed
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));
        coord.requestShutdown(ShutdownReason::UserWindowClose);

        const auto report = coord.executeShutdown(ShutdownReason::UserWindowClose, nullptr, nullptr);

        CHECK(!report.sessionSaved);
        CHECK(!report.databaseClosedCleanly);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }

    // Subcase C: dbCloser returns false (sqlite close issue)
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));
        coord.requestShutdown(ShutdownReason::UserWindowClose);

        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose, []() { return true; }, []() { return false; });

        CHECK(report.sessionSaved);
        CHECK(!report.databaseClosedCleanly);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }
}

void test_lifetime_direct_execute_shutdown() {
    LifetimeConfig config;

    // From Starting directly
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose, []() { return true; }, []() { return true; });
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }

    // From Running directly
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));
        const auto report = coord.executeShutdown(
            ShutdownReason::UserExplicitQuit, []() { return true; }, []() { return true; });
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }

    // From Starting with MemoryExhaustion
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        const auto report = coord.executeShutdown(
            ShutdownReason::MemoryExhaustion, []() { return true; }, []() { return true; });
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 1);
        CHECK(!report.isCleanExit());
        CHECK(!report.sessionSaved);
        CHECK(report.databaseClosedCleanly);
    }
}

void test_lifetime_extreme_timeouts() {
    // Timeout of 0ms: any positive duration triggers timeout
    {
        LifetimeConfig config;
        config.shutdownTimeoutMs = 0;
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));

        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose,
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                return true;
            },
            []() { return true; });

        CHECK(report.timeoutTriggered);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 1);
        CHECK(!report.isCleanExit());
    }

    // Timeout of UINT32_MAX
    {
        LifetimeConfig config;
        config.shutdownTimeoutMs = UINT32_MAX;
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));

        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose, []() { return true; }, []() { return true; });

        CHECK(!report.timeoutTriggered);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }

    // Empty and null paths in LifetimeConfig
    {
        LifetimeConfig config;
        config.sessionFilePath.clear();
        config.profileDirectory.clear();
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));

        const auto report = coord.executeShutdown(
            ShutdownReason::UserWindowClose, []() { return true; }, []() { return true; });

        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }
}

void test_lifetime_concurrent_execute_shutdown() {
    LifetimeConfig config;
    ApplicationLifetimeCoordinator coord(config);
    CHECK(coord.transitionTo(AppLifetimeState::Starting));
    CHECK(coord.transitionTo(AppLifetimeState::Running));

    std::atomic<int> sessionSaverCount{0};
    std::atomic<int> dbCloserCount{0};
    constexpr size_t threadCount = 16;
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    for (size_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([&coord, &sessionSaverCount, &dbCloserCount]() {
            coord.executeShutdown(
                ShutdownReason::UserWindowClose,
                [&sessionSaverCount]() {
                    sessionSaverCount.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    return true;
                },
                [&dbCloserCount]() {
                    dbCloserCount.fetch_add(1, std::memory_order_relaxed);
                    return true;
                });
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    CHECK(sessionSaverCount.load() == 1);
    CHECK(dbCloserCount.load() == 1);
    CHECK(coord.currentState() == AppLifetimeState::Terminated);
}

void test_lifetime_critical_reasons() {
    LifetimeConfig config;

    // MemoryExhaustion: proceeds immediate, skips session flush, calls dbCloser, exits 1
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));

        const auto action = coord.requestShutdown(ShutdownReason::MemoryExhaustion);
        CHECK(action == ShutdownAction::ProceedImmediate);
        CHECK(coord.currentState() == AppLifetimeState::EmergencyAbort);

        bool sessionSaverCalled = false;
        bool dbCloserCalled = false;
        const auto report = coord.executeShutdown(
            ShutdownReason::MemoryExhaustion,
            [&sessionSaverCalled]() {
                sessionSaverCalled = true;
                return true;
            },
            [&dbCloserCalled]() {
                dbCloserCalled = true;
                return true;
            });

        CHECK(!sessionSaverCalled);
        CHECK(!report.sessionSaved);
        CHECK(dbCloserCalled);
        CHECK(report.databaseClosedCleanly);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.exitCode == 1);
        CHECK(!report.isCleanExit());
    }

    // StorageFailure: graceful shutdown flow, records reason
    {
        ApplicationLifetimeCoordinator coord(config);
        CHECK(coord.transitionTo(AppLifetimeState::Starting));
        CHECK(coord.transitionTo(AppLifetimeState::Running));

        const auto action = coord.requestShutdown(ShutdownReason::StorageFailure);
        CHECK(action == ShutdownAction::ProceedGraceful);
        CHECK(coord.currentState() == AppLifetimeState::ShutdownRequested);

        const auto report = coord.executeShutdown(
            ShutdownReason::StorageFailure, []() { return false; }, []() { return true; });

        CHECK(!report.sessionSaved);
        CHECK(report.databaseClosedCleanly);
        CHECK(report.finalState == AppLifetimeState::Terminated);
        CHECK(report.reason == ShutdownReason::StorageFailure);
        CHECK(report.exitCode == 0);
        CHECK(report.isCleanExit());
    }
}

void test_lifetime_timing_metrics() {
    LifetimeConfig config;
    ApplicationLifetimeCoordinator coord(config);
    CHECK(coord.transitionTo(AppLifetimeState::Starting));
    CHECK(coord.transitionTo(AppLifetimeState::Running));

    coord.requestShutdown(ShutdownReason::UserWindowClose);

    const auto report = coord.executeShutdown(
        ShutdownReason::UserWindowClose,
        []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return true;
        },
        []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return true;
        });

    CHECK(report.sessionFlushElapsedMicroseconds >= 1000);
    CHECK(report.teardownElapsedMicroseconds >= 1000);
    CHECK(report.finalState == AppLifetimeState::Terminated);
    CHECK(report.isCleanExit());
}

} // namespace

int main() {
    try {
        test_lifetime_initial_state();
        test_lifetime_transition_matrix();
        test_lifetime_graceful_shutdown();
        test_lifetime_incognito_zero_trace();
        test_lifetime_signal_interrupt_handling();
        test_lifetime_signal_terminate_handling();
        test_lifetime_idempotent_shutdown_requests();
        test_lifetime_repeated_signals_emergency();
        test_lifetime_timeout_watchdog();
        test_lifetime_database_close_cleanly();
        test_lifetime_string_conversions();
        test_lifetime_cancellation_and_recovery();
        test_lifetime_callback_failures_resilience();
        test_lifetime_direct_execute_shutdown();
        test_lifetime_extreme_timeouts();
        test_lifetime_concurrent_execute_shutdown();
        test_lifetime_critical_reasons();
        test_lifetime_timing_metrics();

        std::cout << "Application lifetime tests: ALL PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << '\n';
        return 1;
    }
}
