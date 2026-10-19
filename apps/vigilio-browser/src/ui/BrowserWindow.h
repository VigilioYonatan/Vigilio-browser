#pragma once
#include "core/ApplicationLifetimeCoordinator.h"
#include "core/CrashRecoveryCoordinator.h"
#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutService.h"
#include "core/MainWindowLayoutTypes.h"
#include "core/StartupTypes.h"
#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityService.h"
#include "core/TabStripDensityTypes.h"
#include "core/TitleBarConfiguration.h"
#include "core/TitleBarService.h"
#include "core/TitleBarTypes.h"
#include "storage/LocalRepository.h"
#include <QIcon>
#include <QMainWindow>
#include <QUrl>
#include <memory>
#include <string>
#include <string_view>

class QEvent;
class QLabel;
class QLineEdit;
class QTabBar;
class QTabWidget;
class QTimer;
class QToolBar;
class QWebEngineView;
class QAction;
class QFrame;
class QPushButton;
class QResizeEvent;

class BrowserWindow final : public QMainWindow {
  public:
    explicit BrowserWindow(const vigilio::StartupCommandLine& cmdLine = {},
                           const vigilio::StartupDiagnosticReport& report = {});
    ~BrowserWindow() override;
    void openTab(const QUrl& url = {});
    [[nodiscard]] int tabCount() const;
    [[nodiscard]] QWebEngineView* currentWebView() const;
    void setSeniorMode(bool enabled);
    [[nodiscard]] QIcon makeResourceIcon(std::string_view alias) const;
    void handleSidebarToggle();

    [[nodiscard]] vigilio::MainWindowLayoutService* layoutService() const noexcept {
        return layoutService_.get();
    }
    [[nodiscard]] QFrame* sidebar() const noexcept {
        return sidebar_;
    }
    [[nodiscard]] QPushButton* sidebarToggleButton() const noexcept {
        return sidebarToggleBtn_;
    }
    [[nodiscard]] bool isSidebarExpanded() const noexcept;

    [[nodiscard]] bool isIncognitoSession() const noexcept {
        return isIncognito_;
    }
    [[nodiscard]] bool isSafeModeSession() const noexcept {
        return isSafeMode_;
    }
    [[nodiscard]] QWidget* safeModeBanner() const noexcept {
        return safeModeBanner_;
    }
    [[nodiscard]] QLabel* incognitoBadge() const noexcept {
        return incognitoBadge_;
    }
    [[nodiscard]] vigilio::ApplicationLifetimeCoordinator* lifetimeCoordinator() const noexcept {
        return lifetimeCoordinator_.get();
    }
    [[nodiscard]] QAction* quitAction() const noexcept {
        return quitAction_;
    }
    [[nodiscard]] QWidget* crashRecoveryBanner() const noexcept {
        return crashRecoveryBanner_;
    }
    [[nodiscard]] vigilio::CrashRecoveryService* crashRecoveryService() const noexcept {
        return crashRecoveryService_.get();
    }

    [[nodiscard]] vigilio::TitleBarService* titleBarService() const noexcept {
        return titleBarService_.get();
    }
    [[nodiscard]] QFrame* titleBarWidget() const noexcept {
        return titleBarWidget_;
    }
    [[nodiscard]] QPushButton* titleBarMinimizeButton() const noexcept {
        return titleBarMinimizeBtn_;
    }
    [[nodiscard]] QPushButton* titleBarMaximizeButton() const noexcept {
        return titleBarMaximizeBtn_;
    }
    [[nodiscard]] QPushButton* titleBarCloseButton() const noexcept {
        return titleBarCloseBtn_;
    }
    [[nodiscard]] QPushButton* titleBarSeniorToggleButton() const noexcept {
        return titleBarSeniorToggleBtn_;
    }

    [[nodiscard]] vigilio::TabStripDensityService* tabStripDensityService() const noexcept {
        return tabStripDensityService_.get();
    }
    [[nodiscard]] QTabWidget* tabWidget() const noexcept {
        return tabs_;
    }
    [[nodiscard]] QTabBar* tabBar() const noexcept;
    [[nodiscard]] QPushButton* tabNewButton() const noexcept {
        return tabNewButton_;
    }

  protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

  private:
    friend int main(int argc, char** argv);
    void setupUi();
    void setupLayoutService(const vigilio::StartupDiagnosticReport& report);
    void setupTitleBar(const vigilio::StartupDiagnosticReport& report);
    void setupTabStrip(const vigilio::StartupDiagnosticReport& report);
    void setupSidebar();
    void applyLayoutPlan(const vigilio::MainWindowLayoutPlan& plan);
    void applyTitleBarPlan(const vigilio::TitleBarLayoutPlan& plan);
    void applyTabStripDensityPlan(const vigilio::TabStripDensityPlan& plan);
    void syncTabsToService();
    void updateTitleBarWindowState();
    void setupStartupVisuals(const vigilio::StartupCommandLine& cmdLine,
                             const vigilio::StartupDiagnosticReport& report);
    void setupSafeModeBanner();
    void setupIncognitoBadge();
    void setupCrashRecovery(const vigilio::StartupCommandLine& cmdLine,
                            const vigilio::StartupDiagnosticReport& report);
    void setupCrashRecoveryBanner(const vigilio::CrashRecoveryReport& report);
    void handleCrashRecoveryRestore();
    void handleCrashRecoveryDiscard();
    void handleCrashRecoveryDismiss();
    void navigateFromInput(const QString& text);
    void navigate(const QUrl& url);
    void switchToWebView(const QUrl& url);
    void refreshAddressBar();
    void makeTabMenu(const QPoint& point);
    void closeTab(int index);
    void saveBookmark();
    void showBookmarks();
    void exportSettings();
    void toggleSeniorMode();
    void setDarkTheme(bool enabled);
    void saveSession();
    void restoreSession();
    void activateDeferredTab(int index);
    bool confirmCloseMultipleTabs();
    QWidget* newHomePage();
    QWidget* createErrorPageWidget(const QUrl& failedUrl, QWebEngineView* view = nullptr);

    std::unique_ptr<vigilio::ApplicationLifetimeCoordinator> lifetimeCoordinator_;
    std::unique_ptr<vigilio::CrashRecoveryService> crashRecoveryService_;
    std::unique_ptr<vigilio::MainWindowLayoutService> layoutService_;
    std::unique_ptr<vigilio::TitleBarService> titleBarService_;
    std::unique_ptr<vigilio::TabStripDensityService> tabStripDensityService_;
    QFrame* titleBarWidget_{nullptr};
    QLabel* titleBarIcon_{nullptr};
    QLabel* titleBarTitle_{nullptr};
    QWidget* titleBarDragArea_{nullptr};
    QWidget* titleBarControlsBox_{nullptr};
    QPushButton* titleBarMinimizeBtn_{nullptr};
    QPushButton* titleBarMaximizeBtn_{nullptr};
    QPushButton* titleBarCloseBtn_{nullptr};
    QPushButton* titleBarSeniorToggleBtn_{nullptr};
    QFrame* sidebar_{nullptr};
    QPushButton* sidebarToggleBtn_{nullptr};
    QWidget* mainContainer_{nullptr};
    QAction* toggleSidebarAction_{nullptr};

    QWidget* safeModeBanner_{nullptr};
    QWidget* crashRecoveryBanner_{nullptr};
    QLabel* incognitoBadge_{nullptr};
    QTimer* heartbeatTimer_{nullptr};
    bool isIncognito_{false};
    bool isSafeMode_{false};
    std::string storagePath_;

    std::unique_ptr<vigilio::LocalRepository> db_;
    QTabWidget* tabs_{nullptr};
    QPushButton* tabNewButton_{nullptr};
    QLineEdit* address_{nullptr};
    QToolBar* toolbar_{nullptr};
    QAction* backAction_{nullptr};
    QAction* forwardAction_{nullptr};
    QAction* reloadAction_{nullptr};
    QAction* homeAction_{nullptr};
    QAction* bookmarkAction_{nullptr};
    QAction* favoritesAction_{nullptr};
    QAction* newTabAction_{nullptr};
    QAction* menuAction_{nullptr};
    QAction* seniorAction_{nullptr};
    QAction* themeAction_{nullptr};
    QAction* quitAction_{nullptr};
    bool senior_{false};
    bool dark_{false};
};
