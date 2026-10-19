#pragma once
#include "core/ApplicationLifetimeCoordinator.h"
#include "core/CrashRecoveryCoordinator.h"
#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutService.h"
#include "core/MainWindowLayoutTypes.h"
#include "core/StartupTypes.h"
#include "storage/LocalRepository.h"
#include <QIcon>
#include <QMainWindow>
#include <QUrl>
#include <memory>
#include <string>
#include <string_view>

class QLabel;
class QLineEdit;
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

  protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

  private:
    friend int main(int argc, char** argv);
    void setupUi();
    void setupLayoutService(const vigilio::StartupDiagnosticReport& report);
    void setupSidebar();
    void applyLayoutPlan(const vigilio::MainWindowLayoutPlan& plan);
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
