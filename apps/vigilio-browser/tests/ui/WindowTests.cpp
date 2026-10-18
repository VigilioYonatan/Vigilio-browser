#include "core/ClangFormatConfiguration.h"
#include "core/ClangFormatService.h"
#include "core/ClangFormatTypes.h"
#include "core/ClangFormatValidator.h"
#include "core/CrashRecoveryCoordinator.h"
#include "core/SessionFile.h"
#include "core/TestFixtureFoundation.h"
#include "core/TestFixtureService.h"
#include "core/TestFixtureValidator.h"
#include "ui/BrowserWindow.h"
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSize>
#include <QToolBar>
#include <QUrl>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#define UI_CHECK(cond, msg)                                                                             \
    do {                                                                                                \
        if (!(cond)) {                                                                                  \
            std::cerr << "UI test CHECK failed: " << (msg) << " (" #cond ")\n";                         \
            return 1;                                                                                   \
        }                                                                                               \
    } while (false)

int main(int argc, char** argv) {
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if (!display && !wayland) {
        std::cout
            << "WindowTests: GUI display not available (no DISPLAY / WAYLAND_DISPLAY) - NOT_EXECUTED\n";
        return 0;
    }

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VigilioTests");
    QCoreApplication::setApplicationName("UI-Tests");

    BrowserWindow window;
    UI_CHECK(window.tabCount() >= 1, "Window should initialize with at least one tab or home page");

    const int initialCount = window.tabCount();
    window.openTab();
    UI_CHECK(window.tabCount() == initialCount + 1, "openTab should increment tab count by 1");

    // testWindowIcon
    UI_CHECK(!window.windowIcon().isNull(), "Window windowIcon must be set and not null");

    // testEmptyHomePageCreation and Branding
    UI_CHECK(window.currentWebView() == nullptr, "Empty new tab should not initialize QWebEngineView");
    UI_CHECK(window.findChild<QWidget*>("vigilio-home") != nullptr,
             "New tab should deploy vigilio-home widget");
    auto* homeLogo = window.findChild<QLabel*>("vigilio-home-logo");
    UI_CHECK(homeLogo != nullptr, "Home page must contain vigilio-home-logo");
    UI_CHECK(!homeLogo->pixmap().isNull(), "Home logo must have a non-null pixmap");

    const auto lineEdits = window.findChildren<QLineEdit*>();
    UI_CHECK(!lineEdits.isEmpty(), "Window should contain address / search QLineEdit controls");

    const auto toolbars = window.findChildren<QToolBar*>();
    UI_CHECK(!toolbars.isEmpty(), "Window should contain navigation QToolBar");
    UI_CHECK(window.toolbar_ != nullptr, "Navigation toolbar_ pointer must be valid");

    // testNavigationActionsIcons
    UI_CHECK(!window.backAction_->icon().isNull(), "Back action must have an icon");
    UI_CHECK(!window.forwardAction_->icon().isNull(), "Forward action must have an icon");
    UI_CHECK(!window.reloadAction_->icon().isNull(), "Reload action must have an icon");
    UI_CHECK(!window.homeAction_->icon().isNull(), "Home action must have an icon");
    UI_CHECK(!window.bookmarkAction_->icon().isNull(), "Bookmark action must have an icon");
    UI_CHECK(!window.favoritesAction_->icon().isNull(), "Favorites action must have an icon");
    UI_CHECK(!window.newTabAction_->icon().isNull(), "New tab action must have an icon");
    UI_CHECK(!window.menuAction_->icon().isNull(), "Menu action must have an icon");

    // testAccessibilityMetadata
    UI_CHECK(!window.backAction_->accessibleName().isEmpty(),
             "Back action must have accessibleName defined");
    UI_CHECK(!window.forwardAction_->accessibleName().isEmpty(),
             "Forward action must have accessibleName defined");
    UI_CHECK(!window.reloadAction_->accessibleName().isEmpty(),
             "Reload action must have accessibleName defined");
    UI_CHECK(!window.homeAction_->accessibleName().isEmpty(),
             "Home action must have accessibleName defined");
    UI_CHECK(!window.bookmarkAction_->accessibleName().isEmpty(),
             "Bookmark action must have accessibleName defined");
    UI_CHECK(!window.favoritesAction_->accessibleName().isEmpty(),
             "Favorites action must have accessibleName defined");
    UI_CHECK(!window.newTabAction_->accessibleName().isEmpty(),
             "New tab action must have accessibleName defined");
    UI_CHECK(!window.menuAction_->accessibleName().isEmpty(),
             "Menu action must have accessibleName defined");

    // testShortcutsConfigured
    UI_CHECK(!window.backAction_->shortcut().isEmpty(), "Back action must have a configured shortcut");
    UI_CHECK(!window.forwardAction_->shortcut().isEmpty(),
             "Forward action must have a configured shortcut");
    UI_CHECK(!window.reloadAction_->shortcut().isEmpty(),
             "Reload action must have a configured shortcut");
    UI_CHECK(!window.bookmarkAction_->shortcut().isEmpty(),
             "Bookmark action must have a configured shortcut");
    UI_CHECK(!window.favoritesAction_->shortcut().isEmpty(),
             "Favorites action must have a configured shortcut");
    UI_CHECK(!window.newTabAction_->shortcut().isEmpty(),
             "New tab action must have a configured shortcut");

    // test_ui_lifetime_coordinator
    UI_CHECK(window.lifetimeCoordinator() != nullptr,
             "Window must instantiate ApplicationLifetimeCoordinator");
    UI_CHECK(window.lifetimeCoordinator()->currentState() == vigilio::AppLifetimeState::Running,
             "ApplicationLifetimeCoordinator must transition to Running state on window initialization");
    UI_CHECK(!window.lifetimeCoordinator()->isIncognito(),
             "Standard window lifetime coordinator must not be incognito");
    UI_CHECK(window.lifetimeCoordinator()->configuration().shutdownTimeoutMs == 3000,
             "Shutdown timeout watchdog must be set to 3000 ms");

    // test_quitAction
    UI_CHECK(window.quitAction() != nullptr, "Window must create quitAction");
    UI_CHECK(window.quitAction()->text() == "Cerrar navegador",
             "quitAction text must be 'Cerrar navegador'");
    UI_CHECK(window.quitAction()->shortcut() == QKeySequence::Quit,
             "quitAction shortcut must match QKeySequence::Quit (Ctrl+Q)");
    UI_CHECK(!window.quitAction()->icon().isNull(), "quitAction must have a valid icon");
    UI_CHECK(!window.quitAction()->accessibleName().isEmpty(), "quitAction must define accessibleName");
    UI_CHECK(!window.quitAction()->accessibleDescription().isEmpty(),
             "quitAction must define accessibleDescription");

    // testErrorContainerCreation and Shield Icon
    auto* errorPage = window.createErrorPageWidget(QUrl("https://example.com"), nullptr);
    UI_CHECK(errorPage != nullptr, "Error page widget must be created");
    UI_CHECK(errorPage->objectName() == "vigilio-page-error",
             "Error widget objectName must be vigilio-page-error");

    auto* errorIcon = errorPage->findChild<QLabel*>("vigilio-error-icon");
    UI_CHECK(errorIcon != nullptr, "Error page must contain vigilio-error-icon");
    UI_CHECK(!errorIcon->pixmap().isNull(), "Error page icon must have a non-null pixmap");

    auto* titleLabel = errorPage->findChild<QLabel*>("vigilio-error-title");
    UI_CHECK(titleLabel != nullptr, "Error page must contain vigilio-error-title");
    UI_CHECK(titleLabel->text() == "No se pudo cargar la página", "Title text must match UX copy");

    auto* retryBtn = errorPage->findChild<QPushButton*>("vigilio-error-retry");
    UI_CHECK(retryBtn != nullptr, "Error page must contain vigilio-error-retry button");
    UI_CHECK(retryBtn->text() == "Reintentar", "Retry button text must be Reintentar");

    auto* homeBtn = errorPage->findChild<QPushButton*>("vigilio-error-home");
    UI_CHECK(homeBtn != nullptr, "Error page must contain vigilio-error-home button");
    UI_CHECK(homeBtn->text() == "Ir al inicio", "Home button text must be Ir al inicio");

    // testSeniorModeScaling and iconSize
    window.setSeniorMode(false);
    UI_CHECK(window.toolbar_->iconSize() == QSize(22, 22),
             "Toolbar iconSize must be 22x22 in standard mode");

    window.setSeniorMode(true);
    UI_CHECK(window.toolbar_->iconSize() == QSize(32, 32),
             "Toolbar iconSize must scale to 32x32 in Senior Mode");

    for (auto* le : window.findChildren<QLineEdit*>()) {
        UI_CHECK(le->minimumHeight() >= 48,
                 "QLineEdit minimumHeight >= 48 in accessible interface mode");
    }
    for (auto* btn : window.findChildren<QPushButton*>()) {
        UI_CHECK(btn->minimumHeight() >= 48,
                 "QPushButton minimumHeight >= 48 in accessible interface mode");
    }
    for (auto* tb : window.findChildren<QToolBar*>()) {
        UI_CHECK(tb->minimumHeight() >= 55, "QToolBar minimumHeight >= 55 in accessible interface mode");
    }

    // Reset accessibility mode
    window.setSeniorMode(false);
    UI_CHECK(window.toolbar_->iconSize() == QSize(22, 22),
             "Toolbar iconSize must revert to 22x22 when Senior Mode is disabled");

    // testDeterministicFallback
    const QIcon fallbackIcon = window.makeResourceIcon("icons/nonexistent_test_resource.svg");
    UI_CHECK(!fallbackIcon.isNull(),
             "makeResourceIcon must return valid fallback QIcon for unknown alias");

    errorPage->deleteLater();

    // testStartupSafeModeBanner
    {
        vigilio::StartupCommandLine safeCmd;
        safeCmd.isValid = true;
        safeCmd.isSafeMode = true;
        safeCmd.effectiveMode = vigilio::StartupMode::SafeMode;
        BrowserWindow safeWindow(safeCmd);

        UI_CHECK(safeWindow.isSafeModeSession(), "safeWindow must have isSafeModeSession true");
        UI_CHECK(safeWindow.safeModeBanner() != nullptr, "safeWindow must create safeModeBanner_");
        auto* banner = safeWindow.findChild<QWidget*>("vigilio-safe-mode-banner");
        UI_CHECK(banner != nullptr, "safeWindow must contain vigilio-safe-mode-banner");
        auto* dismissBtn = banner->findChild<QPushButton*>("vigilio-safe-mode-dismiss");
        UI_CHECK(dismissBtn != nullptr, "safeModeBanner must contain vigilio-safe-mode-dismiss button");
        UI_CHECK(dismissBtn->text() == "Entendido", "Dismiss button must display 'Entendido'");
        UI_CHECK(!banner->accessibleName().isEmpty(), "Safe mode banner must have accessibleName");

        // Verify senior mode scaling on safe mode banner
        safeWindow.setSeniorMode(true);
        UI_CHECK(dismissBtn->minimumHeight() >= 48,
                 "Dismiss button minimumHeight >= 48 in accessible interface mode");
        UI_CHECK(banner->minimumHeight() >= 48,
                 "Safe mode banner minimumHeight >= 48 in accessible interface mode");
        safeWindow.setSeniorMode(false);

        // Test dismiss button click
        dismissBtn->click();
        UI_CHECK(!safeWindow.safeModeBanner()->isVisible(),
                 "Safe mode banner must be hidden after clicking dismiss button");
    }

    // testStartupIncognitoMode
    {
        vigilio::StartupCommandLine incogCmd;
        incogCmd.isValid = true;
        incogCmd.isIncognito = true;
        incogCmd.effectiveMode = vigilio::StartupMode::Incognito;
        BrowserWindow incogWindow(incogCmd);

        UI_CHECK(incogWindow.isIncognitoSession(), "incogWindow must have isIncognitoSession true");
        UI_CHECK(incogWindow.incognitoBadge() != nullptr, "incogWindow must create incognitoBadge_");
        auto* badge = incogWindow.findChild<QLabel*>("vigilio-incognito-badge");
        UI_CHECK(badge != nullptr, "incogWindow must contain vigilio-incognito-badge");
        UI_CHECK(badge->text() == "Navegación privada",
                 "Incognito badge text must be 'Navegación privada'");
        UI_CHECK(!badge->accessibleName().isEmpty(), "Incognito badge must have accessibleName");
        UI_CHECK(!badge->accessibleDescription().isEmpty(),
                 "Incognito badge must have accessibleDescription");
        UI_CHECK(incogWindow.windowTitle().contains("Navegación privada"),
                 "Incognito window title must reflect private navigation");
        UI_CHECK(incogWindow.lifetimeCoordinator() != nullptr,
                 "Incognito window must instantiate ApplicationLifetimeCoordinator");
        UI_CHECK(incogWindow.lifetimeCoordinator()->isIncognito(),
                 "Incognito window coordinator must have isIncognito true (Q-001)");
    }

    // testStartupWithInitialUrls
    {
        vigilio::StartupCommandLine urlsCmd;
        urlsCmd.isValid = true;
        urlsCmd.urls = {"https://example.com/one", "https://example.com/two"};
        BrowserWindow urlsWindow(urlsCmd);

        UI_CHECK(urlsWindow.tabCount() == 2,
                 "Window initialized with 2 CLI URLs must create exactly 2 tabs");
    }

    // test_warnOnCloseMultipleTabs
    {
        QSettings settings;
        const bool defaultWarn = settings.value("browser/warnOnCloseMultipleTabs", true).toBool();
        UI_CHECK(defaultWarn == true, "warnOnCloseMultipleTabs must default to true");

        BrowserWindow singleTabWindow;
        UI_CHECK(singleTabWindow.tabCount() == 1,
                 "Single tab window tabCount must be 1 (closes without multi-tab prompt)");

        BrowserWindow multiTabWindow;
        multiTabWindow.openTab();
        UI_CHECK(multiTabWindow.tabCount() >= 2,
                 "Multi-tab window tabCount must be >= 2 for warnOnCloseMultipleTabs verification");
    }

    // test_crash_recovery_banner
    {
        // 1. Abnormal termination detection, banner creation, accessibility, senior scaling, and restore
        // action
        {
            vigilio::TestEnvironmentConfig cfg;
            cfg.prefix = "vigilio_ui_crash_abnormal";
            vigilio::TestFixtureFoundation fixture(cfg);
            UI_CHECK(fixture.isValid(), "Abnormal crash fixture must be valid");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()),
                     "Abnormal crash fixture path must be safe");
            vigilio::CrashRecoveryConfig config;
            config.sentinelFilePath = (fixture.path() / "sentinel.json").string();
            config.sessionFilePath = (fixture.path() / "session.json").string();

            const std::vector<std::string> prevUrls = {"https://example.com/one",
                                                       "https://example.com/two"};
            UI_CHECK(vigilio::saveSession(config.sessionFilePath, prevUrls),
                     "Session save must succeed for crash test");
            UI_CHECK(vigilio::CrashRecoveryCoordinator::markSessionActive(config, 9999, 1000),
                     "Mark session active must succeed");

            vigilio::StartupCommandLine cmd;
            cmd.isValid = true;
            vigilio::StartupDiagnosticReport diag;
            diag.storagePath = fixture.path().string();

            BrowserWindow crashWindow(cmd, diag);

            UI_CHECK(crashWindow.crashRecoveryService() != nullptr,
                     "crashWindow must instantiate CrashRecoveryService");
            UI_CHECK(crashWindow.crashRecoveryBanner() != nullptr,
                     "crashWindow must create crashRecoveryBanner_ upon abnormal termination");
            UI_CHECK(crashWindow.crashRecoveryBanner()->isVisible(),
                     "crashRecoveryBanner_ must be visible upon abnormal termination");

            auto* banner = crashWindow.findChild<QWidget*>("vigilio-crash-recovery-banner");
            UI_CHECK(banner != nullptr, "Window must contain vigilio-crash-recovery-banner child");
            UI_CHECK(!banner->accessibleName().isEmpty(),
                     "Crash recovery banner must have accessibleName defined");
            UI_CHECK(!banner->accessibleDescription().isEmpty(),
                     "Crash recovery banner must have accessibleDescription defined");

            auto* label = banner->findChild<QLabel*>("vigilio-crash-recovery-label");
            UI_CHECK(label != nullptr, "Banner must contain vigilio-crash-recovery-label");
            UI_CHECK(label->text().contains("El navegador no se cerró correctamente"),
                     "Banner label text must match standard abnormal termination copy");

            auto* restoreBtn = banner->findChild<QPushButton*>("vigilio-crash-recovery-restore");
            UI_CHECK(restoreBtn != nullptr, "Banner must contain vigilio-crash-recovery-restore button");
            UI_CHECK(restoreBtn->text() == "Restaurar pestañas",
                     "Restore button text must be 'Restaurar pestañas'");
            UI_CHECK(!restoreBtn->accessibleName().isEmpty(),
                     "Restore button must have accessibleName defined");
            UI_CHECK(restoreBtn->isDefault(),
                     "Restore button must be default action in standard abnormal termination");

            auto* discardBtn = banner->findChild<QPushButton*>("vigilio-crash-recovery-discard");
            UI_CHECK(discardBtn != nullptr, "Banner must contain vigilio-crash-recovery-discard button");
            UI_CHECK(discardBtn->text() == "Empezar de cero",
                     "Discard button text must be 'Empezar de cero'");
            UI_CHECK(!discardBtn->accessibleName().isEmpty(),
                     "Discard button must have accessibleName defined");

            auto* dismissBtn = banner->findChild<QPushButton*>("vigilio-crash-recovery-dismiss");
            UI_CHECK(dismissBtn != nullptr, "Banner must contain vigilio-crash-recovery-dismiss button");
            UI_CHECK(dismissBtn->text() == "Cerrar aviso", "Dismiss button text must be 'Cerrar aviso'");
            UI_CHECK(!dismissBtn->accessibleName().isEmpty(),
                     "Dismiss button must have accessibleName defined");

            // Verify accessible interface mode (Senior Mode) scaling on crash recovery banner
            crashWindow.setSeniorMode(true);
            UI_CHECK(banner->minimumHeight() >= 56,
                     "Crash recovery banner minimumHeight >= 56 in accessible interface mode");
            UI_CHECK(restoreBtn->minimumHeight() >= 48,
                     "Restore button minimumHeight >= 48 in accessible interface mode");
            UI_CHECK(discardBtn->minimumHeight() >= 48,
                     "Discard button minimumHeight >= 48 in accessible interface mode");
            UI_CHECK(dismissBtn->minimumHeight() >= 48,
                     "Dismiss button minimumHeight >= 48 in accessible interface mode");
            crashWindow.setSeniorMode(false);
            UI_CHECK(banner->minimumHeight() == 42, "Crash recovery banner minimumHeight reverts to 42");
            UI_CHECK(restoreBtn->minimumHeight() == 32, "Restore button minimumHeight reverts to 32");

            // Test restore button click
            restoreBtn->click();
            UI_CHECK(!crashWindow.crashRecoveryBanner()->isVisible(),
                     "Crash recovery banner must be hidden after clicking restore button");
            UI_CHECK(crashWindow.tabCount() >= 2, "Restoring session must populate recoverable tabs");
        }

        // 2. Clean exit: banner must NOT be created
        {
            vigilio::TestEnvironmentConfig cfg;
            cfg.prefix = "vigilio_ui_crash_clean";
            vigilio::TestFixtureFoundation fixture(cfg);
            UI_CHECK(fixture.isValid(), "Clean exit fixture must be valid");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()),
                     "Clean exit fixture directory must be safe");
            vigilio::CrashRecoveryConfig config;
            config.sentinelFilePath = (fixture.path() / "sentinel.json").string();
            config.sessionFilePath = (fixture.path() / "session.json").string();

            const std::vector<std::string> prevUrls = {"https://example.com/clean"};
            UI_CHECK(vigilio::saveSession(config.sessionFilePath, prevUrls),
                     "Session save clean exit must succeed");
            UI_CHECK(vigilio::CrashRecoveryCoordinator::markSessionActive(config, 9998, 1000),
                     "Mark active clean exit must succeed");
            UI_CHECK(vigilio::CrashRecoveryCoordinator::markSessionClean(config),
                     "Mark clean exit must succeed");

            vigilio::StartupCommandLine cmd;
            cmd.isValid = true;
            vigilio::StartupDiagnosticReport diag;
            diag.storagePath = fixture.path().string();

            BrowserWindow cleanWindow(cmd, diag);
            UI_CHECK(cleanWindow.crashRecoveryBanner() == nullptr,
                     "Crash recovery banner must not exist upon clean exit");
        }

        // 3. Incognito session isolation (Decisión Q-001)
        {
            vigilio::TestEnvironmentConfig cfg;
            cfg.prefix = "vigilio_ui_crash_incognito";
            vigilio::TestFixtureFoundation fixture(cfg);
            UI_CHECK(fixture.isValid(), "Incognito fixture must be valid");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()),
                     "Incognito fixture directory must be safe");
            vigilio::CrashRecoveryConfig config;
            config.sentinelFilePath = (fixture.path() / "sentinel.json").string();
            config.sessionFilePath = (fixture.path() / "session.json").string();

            UI_CHECK(vigilio::CrashRecoveryCoordinator::markSessionActive(config, 9997, 1000),
                     "Mark active incognito must succeed");

            vigilio::StartupCommandLine incogCmd;
            incogCmd.isValid = true;
            incogCmd.isIncognito = true;
            incogCmd.effectiveMode = vigilio::StartupMode::Incognito;
            vigilio::StartupDiagnosticReport diag;
            diag.storagePath = fixture.path().string();

            BrowserWindow incogWindow(incogCmd, diag);
            UI_CHECK(incogWindow.crashRecoveryService() == nullptr,
                     "Incognito window must not instantiate CrashRecoveryService (Q-001)");
            UI_CHECK(incogWindow.crashRecoveryBanner() == nullptr,
                     "Incognito window must never create crash recovery banner (Q-001)");
        }

        // 4. Crash loop text and discard button click
        {
            vigilio::TestEnvironmentConfig cfg;
            cfg.prefix = "vigilio_ui_crash_loop";
            vigilio::TestFixtureFoundation fixture(cfg);
            UI_CHECK(fixture.isValid(), "Crash loop fixture must be valid");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()),
                     "Crash loop fixture directory must be safe");
            vigilio::CrashRecoveryConfig config;
            config.sentinelFilePath = (fixture.path() / "sentinel.json").string();
            config.sessionFilePath = (fixture.path() / "session.json").string();

            const std::vector<std::string> prevUrls = {"https://example.com/loop"};
            UI_CHECK(vigilio::saveSession(config.sessionFilePath, prevUrls),
                     "Session save loop must succeed");

            {
                std::ofstream f(config.sentinelFilePath, std::ios::trunc);
                f << "{\n"
                  << "  \"schema_version\": 1,\n"
                  << "  \"clean_exit\": false,\n"
                  << "  \"consecutive_crashes\": 2,\n"
                  << "  \"pid\": 9996,\n"
                  << "  \"session_start_time\": 1000,\n"
                  << "  \"last_heartbeat_time\": 1010,\n"
                  << "  \"last_crash_reason\": 1,\n"
                  << "  \"app_version\": \"0.1.0-alpha\"\n"
                  << "}\n";
            }

            vigilio::StartupCommandLine cmd;
            cmd.isValid = true;
            vigilio::StartupDiagnosticReport diag;
            diag.storagePath = fixture.path().string();

            BrowserWindow loopWindow(cmd, diag);
            UI_CHECK(loopWindow.crashRecoveryBanner() != nullptr, "Crash loop banner must be created");
            auto* label =
                loopWindow.crashRecoveryBanner()->findChild<QLabel*>("vigilio-crash-recovery-label");
            UI_CHECK(label != nullptr, "Crash loop banner must contain label");
            UI_CHECK(label->text().contains("varias veces"),
                     "Crash loop banner label text must warn about repeated crashes");

            auto* discardBtn = loopWindow.crashRecoveryBanner()->findChild<QPushButton*>(
                "vigilio-crash-recovery-discard");
            UI_CHECK(discardBtn != nullptr,
                     "Crash loop banner must contain vigilio-crash-recovery-discard button");
            UI_CHECK(discardBtn->isDefault(), "Discard button must be default action in crash loop");

            // Test discard button click
            discardBtn->click();
            UI_CHECK(!loopWindow.crashRecoveryBanner()->isVisible(),
                     "Crash recovery banner must be hidden after clicking discard button");
        }

        // 5. Dismiss / cancel action
        {
            vigilio::TestEnvironmentConfig cfg;
            cfg.prefix = "vigilio_ui_crash_dismiss";
            vigilio::TestFixtureFoundation fixture(cfg);
            UI_CHECK(fixture.isValid(), "Dismiss fixture must be valid");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(fixture.path()),
                     "Dismiss fixture directory must be safe");
            vigilio::CrashRecoveryConfig config;
            config.sentinelFilePath = (fixture.path() / "sentinel.json").string();
            config.sessionFilePath = (fixture.path() / "session.json").string();

            UI_CHECK(vigilio::CrashRecoveryCoordinator::markSessionActive(config, 9995, 1000),
                     "Mark active dismiss must succeed");

            vigilio::StartupCommandLine cmd;
            cmd.isValid = true;
            vigilio::StartupDiagnosticReport diag;
            diag.storagePath = fixture.path().string();

            BrowserWindow dismissWindow(cmd, diag);
            UI_CHECK(dismissWindow.crashRecoveryBanner() != nullptr,
                     "Dismiss window must create crash recovery banner");
            auto* dismissBtn = dismissWindow.crashRecoveryBanner()->findChild<QPushButton*>(
                "vigilio-crash-recovery-dismiss");
            UI_CHECK(dismissBtn != nullptr, "Dismiss button must exist");

            // Test dismiss button click
            dismissBtn->click();
            UI_CHECK(!dismissWindow.crashRecoveryBanner()->isVisible(),
                     "Crash recovery banner must be hidden after clicking dismiss button");
        }

        // 6. Test fixture service integration: profile seeding and snapshots in UI context
        {
            vigilio::TestFixtureService fixtureService;
            auto* managed = fixtureService.createManagedFixture("ui_managed_profile");
            UI_CHECK(managed != nullptr, "Managed fixture must be created");
            UI_CHECK(vigilio::TestFixtureValidator::isSafeTestDirectory(managed->path()),
                     "Managed fixture path must be safe");

            vigilio::BrowserProfileSeedOptions seedOpts;
            seedOpts.profileName = "ui_test_profile";
            const auto seedRes = fixtureService.seedBrowserProfile(*managed, seedOpts);
            UI_CHECK(seedRes.isSuccess(), "Seeding browser profile for UI test must succeed");

            const auto snapRes = fixtureService.createSnapshot(*managed, "ui_initial_state");
            UI_CHECK(snapRes.isSuccess(), "Creating UI test snapshot must succeed");

            const auto rollbackRes = fixtureService.rollbackToSnapshot(*managed, "ui_initial_state");
            UI_CHECK(rollbackRes.isSuccess(), "Rollback to UI test snapshot must succeed");

            fixtureService.releaseAllFixtures();
            UI_CHECK(fixtureService.activeFixtureCount() == 0,
                     "All managed UI fixtures must be released");
        }

        // 7. Clang-Format quality governance and public copy audit in UI context
        {
            std::filesystem::path browserRoot;
#ifdef VIGILIO_BROWSER_ROOT
            browserRoot = VIGILIO_BROWSER_ROOT;
#endif
            if (browserRoot.empty() ||
                !std::filesystem::exists(browserRoot / "src/ui/BrowserWindow.h")) {
                for (const auto& candidate :
                     {std::filesystem::current_path(), std::filesystem::current_path().parent_path(),
                      std::filesystem::current_path() / "apps/vigilio-browser"}) {
                    if (std::filesystem::exists(candidate / "src/ui/BrowserWindow.h")) {
                        browserRoot = candidate;
                        break;
                    }
                }
            }
            if (!browserRoot.empty() && std::filesystem::exists(browserRoot)) {
                browserRoot = std::filesystem::canonical(browserRoot);
            }
            UI_CHECK(vigilio::ClangFormatValidator::isSafeConfigPath(browserRoot),
                     "Resolved browserRoot path must be safe and contained");

            vigilio::ClangFormatService formatService(
                vigilio::ClangFormatConfiguration::canonicalConfig());
            UI_CHECK(formatService.isConfigured(), "ClangFormatService must be configured");

            vigilio::FormatServiceOptions serviceOptions;
            serviceOptions.dryRun = true;
            serviceOptions.maxFileSizeBytes = 10 * 1024 * 1024;

            // TASK-003: Programmatic style inspection (dry-run) for BrowserWindow.h
            const auto headerInspection =
                formatService.inspectFile(browserRoot / "src/ui/BrowserWindow.h");
            const auto headerStatus = headerInspection.isCompliant
                                          ? vigilio::FormatExecutionStatus::Success
                                          : vigilio::FormatExecutionStatus::DiscrepancyFound;
            UI_CHECK(headerStatus == vigilio::FormatExecutionStatus::Success,
                     "BrowserWindow.h must comply with Clang-Format canonical style");
            UI_CHECK(headerInspection.isCompliant, "BrowserWindow.h isCompliant must be true");
            UI_CHECK(headerInspection.discrepancySummary.empty(),
                     "BrowserWindow.h must have zero discrepancies");

            // TASK-004: Programmatic style inspection (dry-run) for BrowserWindow.cpp
            const auto implInspection =
                formatService.inspectFile(browserRoot / "src/ui/BrowserWindow.cpp");
            const auto implStatus = implInspection.isCompliant
                                        ? vigilio::FormatExecutionStatus::Success
                                        : vigilio::FormatExecutionStatus::DiscrepancyFound;
            UI_CHECK(implStatus == vigilio::FormatExecutionStatus::Success,
                     "BrowserWindow.cpp must comply with Clang-Format canonical style");
            UI_CHECK(implInspection.isCompliant, "BrowserWindow.cpp isCompliant must be true");
            UI_CHECK(implInspection.discrepancySummary.empty(),
                     "BrowserWindow.cpp must have zero discrepancies");

            // TASK-005: Programmatic style inspection (dry-run) for WindowTests.cpp
            const auto testsInspection =
                formatService.inspectFile(browserRoot / "tests/ui/WindowTests.cpp");
            const auto testsStatus = testsInspection.isCompliant
                                         ? vigilio::FormatExecutionStatus::Success
                                         : vigilio::FormatExecutionStatus::DiscrepancyFound;
            UI_CHECK(testsStatus == vigilio::FormatExecutionStatus::Success,
                     "WindowTests.cpp must comply with Clang-Format canonical style");
            UI_CHECK(testsInspection.isCompliant, "WindowTests.cpp isCompliant must be true");
            UI_CHECK(testsInspection.discrepancySummary.empty(),
                     "WindowTests.cpp must have zero discrepancies");

            // TASK-006: Audit of diagnostic and remediation copy in neutral Spanish (UX-COPY-PUBLICO.md)
            const auto successMsg = vigilio::ClangFormatService::executionStatusToString(
                vigilio::FormatExecutionStatus::Success);
            UI_CHECK(!successMsg.empty(), "Success message must not be empty");
            UI_CHECK(successMsg.find("correctamente") != std::string::npos ||
                         successMsg.find("éxito") != std::string::npos,
                     "Success message must be clear natural Spanish");

            const auto discrepancyMsg = vigilio::ClangFormatService::executionStatusToString(
                vigilio::FormatExecutionStatus::DiscrepancyFound);
            UI_CHECK(!discrepancyMsg.empty(), "DiscrepancyFound message must not be empty");
            UI_CHECK(discrepancyMsg.find("diferencias") != std::string::npos ||
                         discrepancyMsg.find("discrepancias") != std::string::npos,
                     "DiscrepancyFound message must be clear natural Spanish");

            const auto cancelledMsg = vigilio::ClangFormatService::executionStatusToString(
                vigilio::FormatExecutionStatus::Cancelled);
            UI_CHECK(!cancelledMsg.empty(), "Cancelled message must not be empty");
            UI_CHECK(cancelledMsg.find("cancelada") != std::string::npos,
                     "Cancelled message must be clear natural Spanish");

            const auto lockedMsg = vigilio::ClangFormatService::executionStatusToString(
                vigilio::FormatExecutionStatus::FileLocked);
            UI_CHECK(!lockedMsg.empty(), "FileLocked message must not be empty");
            UI_CHECK(lockedMsg.find("archivo") != std::string::npos,
                     "FileLocked message must be clear natural Spanish");

            // Verify absence of forbidden internal technical jargon across all status strings
            const std::vector<vigilio::FormatExecutionStatus> sampleStatuses = {
                vigilio::FormatExecutionStatus::Success,
                vigilio::FormatExecutionStatus::DiscrepancyFound,
                vigilio::FormatExecutionStatus::Cancelled,
                vigilio::FormatExecutionStatus::FileLocked,
                vigilio::FormatExecutionStatus::PermissionDenied,
                vigilio::FormatExecutionStatus::ReadError,
                vigilio::FormatExecutionStatus::WriteError,
                vigilio::FormatExecutionStatus::FileTooLarge,
                vigilio::FormatExecutionStatus::UnsafePath,
                vigilio::FormatExecutionStatus::FileNotFound,
                vigilio::FormatExecutionStatus::ConfigurationError,
                vigilio::FormatExecutionStatus::ToolUnavailable};
            for (const auto st : sampleStatuses) {
                const auto statusStr = vigilio::ClangFormatService::executionStatusToString(st);
                UI_CHECK(statusStr.find("Senior Mode") == std::string::npos,
                         "Public status string must not contain 'Senior Mode'");
                UI_CHECK(statusStr.find("Modo Senior") == std::string::npos,
                         "Public status string must not contain 'Modo Senior'");
                UI_CHECK(statusStr.find("Prompt Builder") == std::string::npos,
                         "Public status string must not contain 'Prompt Builder'");
                UI_CHECK(statusStr.find("OpenSpec") == std::string::npos,
                         "Public status string must not contain 'OpenSpec'");
                UI_CHECK(statusStr.find("Linter") == std::string::npos,
                         "Public status string must not contain 'Linter'");
                UI_CHECK(statusStr.find("Clang-Format") == std::string::npos,
                         "Public status string must not contain 'Clang-Format'");
            }
        }
    }

    std::cout << "UI interaction tests: PASS\n";
    return 0;
}
