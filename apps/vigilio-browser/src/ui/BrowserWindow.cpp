#include "ui/BrowserWindow.h"
#include "core/ResourceInclusionService.h"
#include "core/UrlInput.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWidget>
#include <chrono>
#include <cstdint>

namespace {
QString profileLocation() {
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}
constexpr auto startTitle = "Nueva pestaña";
} // namespace

BrowserWindow::BrowserWindow(const vigilio::StartupCommandLine& cmdLine,
                             const vigilio::StartupDiagnosticReport& report) {
    isIncognito_ = cmdLine.isIncognito;
    isSafeMode_ = cmdLine.isSafeMode;

    // When cmdLine.isIncognito is true, persistence is isolated to ":memory:"
    if (cmdLine.isIncognito) {
        storagePath_ = ":memory:";
    } else if (!report.storagePath.empty()) {
        storagePath_ = report.storagePath + "/browser.sqlite";
    } else {
        storagePath_ = (profileLocation() + "/browser.sqlite").toStdString();
    }
    db_ = std::make_unique<vigilio::LocalRepository>(storagePath_);

    std::string profileDir;
    if (!report.storagePath.empty()) {
        profileDir = report.storagePath;
    } else {
        profileDir = profileLocation().toStdString();
    }

    vigilio::LifetimeConfig lifetimeConfig;
    lifetimeConfig.isIncognito = isIncognito_;
    lifetimeConfig.isSafeMode = isSafeMode_;
    lifetimeConfig.shutdownTimeoutMs = 3000;
    lifetimeConfig.profileDirectory = profileDir;
    lifetimeConfig.sessionFilePath = profileDir + "/session.json";
    lifetimeCoordinator_ = std::make_unique<vigilio::ApplicationLifetimeCoordinator>(lifetimeConfig);

    setupUi();
    setupStartupVisuals(cmdLine, report);
    setupCrashRecovery(cmdLine, report);

    QSettings settings;
    dark_ = settings.value("appearance/dark", false).toBool();
    setDarkTheme(dark_);
    senior_ = settings.value("accessibility/senior", false).toBool();
    setSeniorMode(senior_);

    if (!cmdLine.urls.empty()) {
        for (const auto& rawUrl : cmdLine.urls) {
            const QUrl url(QString::fromStdString(rawUrl));
            if (url.isValid() && !url.scheme().isEmpty()) {
                openTab(url);
            } else {
                openTab(QUrl::fromUserInput(QString::fromStdString(rawUrl)));
            }
        }
        if (tabs_->count() > 0) {
            tabs_->setCurrentIndex(0);
        }
    } else {
        if (!crashRecoveryBanner_) {
            restoreSession();
        }
        if (tabs_->count() == 0) {
            openTab();
        }
    }

    resize(1200, 780);
    if (isIncognito_) {
        statusBar()->showMessage("Vigilio Browser · Navegación privada · No se guardan datos locales",
                                 5000);
    } else if (isSafeMode_) {
        statusBar()->showMessage("Vigilio Browser · Modo seguro · Extensiones desactivadas", 5000);
    } else {
        statusBar()->showMessage("Vigilio Browser · Datos locales · Sin cuenta", 4000);
    }

    lifetimeCoordinator_->transitionTo(vigilio::AppLifetimeState::Running);
}
BrowserWindow::~BrowserWindow() = default;

QIcon BrowserWindow::makeResourceIcon(std::string_view alias) const {
    const std::string qrcPath = vigilio::ResourceInclusionService::resolveIconWithFallback(alias);
    return QIcon(QString::fromStdString(qrcPath));
}

void BrowserWindow::setupUi() {
    setWindowTitle("Vigilio Browser");
    setWindowIcon(makeResourceIcon("branding/vigilio-logo.svg"));
    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    tabs_ = new QTabWidget(central);
    tabs_->setObjectName("vigilio-tabs");
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    tabs_->setDocumentMode(true);
    centralLayout->addWidget(tabs_);
    setCentralWidget(central);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        refreshAddressBar();
        QTimer::singleShot(0, this, [this, index]() { activateDeferredTab(index); });
    });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) { closeTab(index); });
    auto* bar = tabs_->tabBar();
    bar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(bar, &QWidget::customContextMenuRequested, this,
            [this](const QPoint& p) { makeTabMenu(p); });

    toolbar_ = addToolBar("Navegación");
    toolbar_->setMovable(false);
    toolbar_->setObjectName("vigilio-navigation");
    toolbar_->setIconSize(QSize(22, 22));

    backAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/back.svg"), "Atrás");
    backAction_->setObjectName("vigilio-action-back");
    backAction_->setShortcut(QKeySequence::Back);
    backAction_->setAccessibleName("Retroceder una página");
    backAction_->setAccessibleDescription("Volver a la página web anterior en el historial");

    forwardAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/forward.svg"), "Adelante");
    forwardAction_->setObjectName("vigilio-action-forward");
    forwardAction_->setShortcut(QKeySequence::Forward);
    forwardAction_->setAccessibleName("Avanzar una página");
    forwardAction_->setAccessibleDescription("Avanzar a la siguiente página web en el historial");

    reloadAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/reload.svg"), "Recargar");
    reloadAction_->setObjectName("vigilio-action-reload");
    reloadAction_->setShortcut(QKeySequence::Refresh);
    reloadAction_->setAccessibleName("Recargar página actual");
    reloadAction_->setAccessibleDescription("Actualizar el contenido de la página web actual");

    homeAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/home.svg"), "Inicio");
    homeAction_->setObjectName("vigilio-action-home");
    homeAction_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Home));
    homeAction_->setAccessibleName("Ir a la página de inicio");
    homeAction_->setAccessibleDescription("Cargar la página de inicio de Vigilio Browser");

    address_ = new QLineEdit(this);
    address_->setObjectName("vigilio-address");
    address_->setPlaceholderText("Buscar o escribir una dirección web");
    address_->setAccessibleName("Barra de direcciones y búsqueda");
    address_->setAccessibleDescription(
        "Escribir una dirección URL o un término de búsqueda y presionar Enter");
    toolbar_->addWidget(address_);

    bookmarkAction_ =
        toolbar_->addAction(makeResourceIcon("icons/navigation/bookmarks.svg"), "Favorito");
    bookmarkAction_->setObjectName("vigilio-action-bookmark");
    bookmarkAction_->setShortcut(QKeySequence("Ctrl+D"));
    bookmarkAction_->setAccessibleName("Guardar página actual en favoritos");
    bookmarkAction_->setAccessibleDescription(
        "Añadir la página web abierta a la lista de marcadores favoritos");

    favoritesAction_ =
        toolbar_->addAction(makeResourceIcon("icons/navigation/bookmarks.svg"), "Favoritos");
    favoritesAction_->setObjectName("vigilio-action-favorites");
    favoritesAction_->setShortcut(QKeySequence("Ctrl+B"));
    favoritesAction_->setAccessibleName("Mostrar lista de marcadores favoritos");
    favoritesAction_->setAccessibleDescription("Abrir la ventana de páginas web guardadas en favoritos");

    newTabAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/tabs.svg"), "Nueva pestaña");
    newTabAction_->setObjectName("vigilio-action-new-tab");
    newTabAction_->setShortcut(QKeySequence("Ctrl+T"));
    newTabAction_->setAccessibleName("Abrir una nueva pestaña");
    newTabAction_->setAccessibleDescription("Crear una nueva pestaña en blanco para navegar");

    menuAction_ = toolbar_->addAction(makeResourceIcon("icons/navigation/settings.svg"), "Menú");
    menuAction_->setObjectName("vigilio-action-menu");
    menuAction_->setShortcut(QKeySequence("Alt+F"));
    menuAction_->setAccessibleName("Abrir menú principal de opciones");
    menuAction_->setAccessibleDescription(
        "Desplegar las opciones de configuración, tema y accesibilidad");

    connect(backAction_, &QAction::triggered, this, [this]() {
        if (auto* v = currentWebView()) {
            v->back();
        }
    });
    connect(forwardAction_, &QAction::triggered, this, [this]() {
        if (auto* v = currentWebView()) {
            v->forward();
        }
    });
    connect(reloadAction_, &QAction::triggered, this, [this]() {
        if (auto* v = currentWebView()) {
            v->reload();
        } else if (auto* current = tabs_->currentWidget()) {
            if (auto* retry = current->findChild<QPushButton*>("vigilio-error-retry")) {
                retry->click();
            }
        }
    });
    connect(homeAction_, &QAction::triggered, this, [this]() { openTab(); });
    connect(address_, &QLineEdit::returnPressed, this,
            [this]() { navigateFromInput(address_->text()); });
    connect(bookmarkAction_, &QAction::triggered, this, [this]() { saveBookmark(); });
    connect(favoritesAction_, &QAction::triggered, this, [this]() { showBookmarks(); });
    connect(newTabAction_, &QAction::triggered, this, [this]() { openTab(); });

    auto* menu = new QMenu(this);
    seniorAction_ =
        menu->addAction(makeResourceIcon("icons/tools/senior.svg"), "Interfaz más fácil de usar");
    seniorAction_->setObjectName("vigilio-action-senior");
    seniorAction_->setCheckable(true);
    seniorAction_->setAccessibleName(
        "Activar o desactivar interfaz más fácil de usar con letras y botones grandes");
    seniorAction_->setAccessibleDescription("Conmuta el modo de visualización accesible");
    connect(seniorAction_, &QAction::triggered, this, [this]() { toggleSeniorMode(); });

    themeAction_ = menu->addAction(makeResourceIcon("icons/navigation/settings.svg"), "Tema oscuro");
    themeAction_->setObjectName("vigilio-action-theme");
    themeAction_->setCheckable(true);
    themeAction_->setAccessibleName("Tema oscuro");
    themeAction_->setAccessibleDescription("Cambiar entre tema claro y tema oscuro");
    connect(themeAction_, &QAction::triggered, this, [this](bool checked) { setDarkTheme(checked); });

    menu->addSeparator();
    auto* exportAction = menu->addAction("Exportar configuración local…");
    connect(exportAction, &QAction::triggered, this, [this]() { exportSettings(); });
    auto* privacyInfo = menu->addAction("Privacidad y características pendientes");
    connect(privacyInfo, &QAction::triggered, this, [this]() {
        QMessageBox::information(
            this, "Vigilio 0.1",
            "Los datos del navegador se guardan localmente. No hay cuenta Vigilio ni nube. "
            "AdBlock, MCP, extensiones, modo privado y otros visores siguen pendientes de "
            "implementación.");
    });

    quitAction_ = new QAction(makeResourceIcon("icons/tools/log-out.svg"), "Cerrar navegador", this);
    quitAction_->setObjectName("vigilio-action-quit");
    quitAction_->setShortcut(QKeySequence::Quit);
    quitAction_->setAccessibleName("Cerrar navegador");
    quitAction_->setAccessibleDescription("Cerrar todas las pestañas y salir de la aplicación");
    connect(quitAction_, &QAction::triggered, this, &BrowserWindow::close);
    addAction(quitAction_);
    menu->addSeparator();
    menu->addAction(quitAction_);
    connect(menuAction_, &QAction::triggered, this, [this, menu]() {
        const auto button = toolbar_->widgetForAction(menuAction_);
        if (button) {
            menu->popup(button->mapToGlobal(QPoint(0, button->height())));
        } else {
            menu->popup(QCursor::pos());
        }
    });

    auto* closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(closeShortcut, &QShortcut::activated, this, [this]() { closeTab(tabs_->currentIndex()); });
    auto* addressShortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(addressShortcut, &QShortcut::activated, this, [this]() {
        address_->setFocus();
        address_->selectAll();
    });
}

void BrowserWindow::setupStartupVisuals(const vigilio::StartupCommandLine& /*cmdLine*/,
                                        const vigilio::StartupDiagnosticReport& /*report*/) {
    if (isSafeMode_) {
        setupSafeModeBanner();
    }
    if (isIncognito_) {
        setWindowTitle("Vigilio Browser (Navegación privada)");
        setupIncognitoBadge();
    }
}

void BrowserWindow::setupSafeModeBanner() {
    auto* banner = new QFrame(this);
    banner->setObjectName("vigilio-safe-mode-banner");
    banner->setAccessibleName("Aviso de modo seguro");
    banner->setAccessibleDescription(
        "Informa que el navegador se inició en modo seguro a prueba de fallos");
    banner->setMinimumHeight(42);

    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(8, 8, 8, 8);
    bannerLayout->setSpacing(8);

    auto* safeIcon = new QLabel(banner);
    safeIcon->setObjectName("vigilio-safe-mode-icon");
    safeIcon->setPixmap(makeResourceIcon("icons/tools/shield.svg").pixmap(20, 20));

    auto* safeLabel = new QLabel("Modo seguro activado: extensiones y configuraciones experimentales "
                                 "desactivadas para proteger la estabilidad",
                                 banner);
    safeLabel->setObjectName("vigilio-safe-mode-label");

    auto* dismissBtn = new QPushButton("Entendido", banner);
    dismissBtn->setObjectName("vigilio-safe-mode-dismiss");
    dismissBtn->setAccessibleName("Entendido");
    dismissBtn->setAccessibleDescription("Descartar aviso de modo seguro");
    dismissBtn->setMinimumHeight(32);
    dismissBtn->setMinimumWidth(80);

    connect(dismissBtn, &QPushButton::clicked, this, [this]() {
        if (safeModeBanner_) {
            safeModeBanner_->hide();
        }
        if (address_) {
            address_->setFocus();
        }
    });

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), banner);
    connect(escShortcut, &QShortcut::activated, dismissBtn, &QPushButton::click);

    bannerLayout->addWidget(safeIcon);
    bannerLayout->addWidget(safeLabel, 1);
    bannerLayout->addWidget(dismissBtn);

    safeModeBanner_ = banner;
    if (auto* central = centralWidget()) {
        if (auto* lay = qobject_cast<QVBoxLayout*>(central->layout())) {
            lay->insertWidget(0, safeModeBanner_);
        }
    }
}

void BrowserWindow::setupIncognitoBadge() {
    incognitoBadge_ = new QLabel(this);
    incognitoBadge_->setObjectName("vigilio-incognito-badge");
    incognitoBadge_->setText("Navegación privada");
    incognitoBadge_->setAccessibleName("Navegación privada activa");
    incognitoBadge_->setAccessibleDescription(
        "No se guardará el historial de navegación ni datos en este equipo");
    makeResourceIcon("icons/tools/shield.svg");
    statusBar()->addPermanentWidget(incognitoBadge_);
}

void BrowserWindow::setupCrashRecovery(const vigilio::StartupCommandLine& /*cmdLine*/,
                                       const vigilio::StartupDiagnosticReport& report) {
    if (isIncognito_) {
        return;
    }

    std::string profileDir;
    if (!report.storagePath.empty()) {
        profileDir = report.storagePath;
    } else {
        profileDir = profileLocation().toStdString();
    }

    vigilio::CrashRecoveryConfig config;
    config.isIncognito = isIncognito_;
    config.isSafeMode = isSafeMode_;
    config.crashLoopThreshold = 2;
    config.sessionAutosaveIntervalMs = 30000;
    config.profileDirectory = profileDir;
    config.sentinelFilePath = profileDir + "/sentinel.json";
    config.sessionFilePath = profileDir + "/session.json";

    crashRecoveryService_ = std::make_unique<vigilio::CrashRecoveryService>(config);

    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    const uint64_t pid = static_cast<uint64_t>(QCoreApplication::applicationPid());

    // initialize CrashRecoveryService for abnormal termination inspection
    const auto crashReport = crashRecoveryService_->initialize(config, pid, now);

    if (crashReport.state == vigilio::CrashDetectionState::AbnormalTermination ||
        crashReport.state == vigilio::CrashDetectionState::RepeatedCrashes ||
        crashReport.recommendedAction == vigilio::RecoveryAction::PromptRestore ||
        crashReport.recommendedAction == vigilio::RecoveryAction::SafeModeRecovery) {
        setupCrashRecoveryBanner(crashReport);
    }

    heartbeatTimer_ = new QTimer(this);
    connect(heartbeatTimer_, &QTimer::timeout, this, [this]() {
        if (crashRecoveryService_) {
            const auto currentNow = std::chrono::duration_cast<std::chrono::seconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count();
            crashRecoveryService_->recordHeartbeat(currentNow);
        }
    });
    heartbeatTimer_->start(30000);
}

void BrowserWindow::setupCrashRecoveryBanner(const vigilio::CrashRecoveryReport& report) {
    auto* banner = new QFrame(this);
    banner->setObjectName("vigilio-crash-recovery-banner");
    banner->setAccessibleName("Aviso de recuperación tras cierre inesperado");
    banner->setAccessibleDescription(
        "Permite restaurar las pestañas abiertas antes del cierre anómalo del navegador o empezar con "
        "una sesión limpia");
    banner->setMinimumHeight(senior_ ? 56 : 42);

    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(12, 8, 12, 8);
    bannerLayout->setSpacing(10);

    auto* iconLabel = new QLabel(banner);
    iconLabel->setObjectName("vigilio-crash-recovery-icon");
    iconLabel->setAccessibleName("Icono de recuperación tras caída");
    iconLabel->setPixmap(
        makeResourceIcon("icons/tools/shield.svg").pixmap(senior_ ? 28 : 20, senior_ ? 28 : 20));

    const bool isCrashLoop = (report.consecutiveCrashCount >= 2 ||
                              report.state == vigilio::CrashDetectionState::RepeatedCrashes);

    const QString messageText =
        isCrashLoop ? "El navegador se cerró varias veces de forma inesperada. Se recomienda abrir "
                      "una sesión limpia para mayor estabilidad."
                    : "El navegador no se cerró correctamente. ¿Quieres restaurar tus pestañas "
                      "anteriores?";

    auto* messageLabel = new QLabel(messageText, banner);
    messageLabel->setObjectName("vigilio-crash-recovery-label");
    messageLabel->setAccessibleName(messageText);

    auto* restoreBtn = new QPushButton(
        isCrashLoop ? "Restaurar pestañas de todos modos" : "Restaurar pestañas", banner);
    restoreBtn->setObjectName("vigilio-crash-recovery-restore");
    restoreBtn->setAccessibleName(restoreBtn->text());
    restoreBtn->setAccessibleDescription(
        "Restaurar las pestañas previas que estaban abiertas en la sesión anterior");
    restoreBtn->setMinimumHeight(senior_ ? 48 : 32);

    auto* discardBtn = new QPushButton("Empezar de cero", banner);
    discardBtn->setObjectName("vigilio-crash-recovery-discard");
    discardBtn->setAccessibleName("Empezar de cero");
    discardBtn->setAccessibleDescription("Descartar la sesión anterior e iniciar una navegación limpia");
    discardBtn->setMinimumHeight(senior_ ? 48 : 32);

    auto* dismissBtn = new QPushButton("Cerrar aviso", banner);
    dismissBtn->setObjectName("vigilio-crash-recovery-dismiss");
    dismissBtn->setAccessibleName("Cerrar aviso");
    dismissBtn->setAccessibleDescription(
        "Ocultar el aviso de recuperación sin borrar la sesión anterior");
    dismissBtn->setMinimumHeight(senior_ ? 48 : 32);

    if (isCrashLoop) {
        discardBtn->setDefault(true);
    } else {
        restoreBtn->setDefault(true);
    }

    connect(restoreBtn, &QPushButton::clicked, this, &BrowserWindow::handleCrashRecoveryRestore);
    connect(discardBtn, &QPushButton::clicked, this, &BrowserWindow::handleCrashRecoveryDiscard);
    connect(dismissBtn, &QPushButton::clicked, this, &BrowserWindow::handleCrashRecoveryDismiss);

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), banner);
    connect(escShortcut, &QShortcut::activated, dismissBtn, &QPushButton::click);

    bannerLayout->addWidget(iconLabel);
    bannerLayout->addWidget(messageLabel, 1);
    if (isCrashLoop) {
        bannerLayout->addWidget(discardBtn);
        bannerLayout->addWidget(restoreBtn);
    } else {
        bannerLayout->addWidget(restoreBtn);
        bannerLayout->addWidget(discardBtn);
    }
    bannerLayout->addWidget(dismissBtn);

    crashRecoveryBanner_ = banner;
    if (auto* central = centralWidget()) {
        if (auto* lay = qobject_cast<QVBoxLayout*>(central->layout())) {
            const int insertIndex = safeModeBanner_ ? 1 : 0;
            lay->insertWidget(insertIndex, crashRecoveryBanner_);
        }
    }
}

void BrowserWindow::handleCrashRecoveryRestore() {
    if (!crashRecoveryService_) {
        if (crashRecoveryBanner_) {
            crashRecoveryBanner_->hide();
        }
        return;
    }

    const auto urls = crashRecoveryService_->applyRecovery();
    if (crashRecoveryBanner_) {
        crashRecoveryBanner_->hide();
    }

    if (urls.empty()) {
        if (tabs_->count() == 0) {
            openTab();
        }
        statusBar()->showMessage("Pestañas restauradas con éxito", 3000);
        return;
    }

    bool hadOnlyEmptyHome = false;
    if (tabs_->count() == 1 && tabs_->widget(0)->findChild<QWidget*>("vigilio-home") != nullptr) {
        hadOnlyEmptyHome = true;
    }

    bool first = true;
    for (const auto& rawUrl : urls) {
        const QUrl url(QString::fromStdString(rawUrl));
        if (url.scheme() != "http" && url.scheme() != "https") {
            continue;
        }

        if (first) {
            if (hadOnlyEmptyHome) {
                QSignalBlocker blocker(tabs_);
                auto* old = tabs_->widget(0);
                tabs_->removeTab(0);
                old->deleteLater();
            }
            openTab(url);
            first = false;
        } else {
            auto* placeholder = new QWidget(tabs_);
            placeholder->setProperty("deferredUrl", QString::fromStdString(rawUrl));
            auto* layout = new QVBoxLayout(placeholder);
            layout->addWidget(new QLabel("Pestaña en espera. Se cargará al abrirla.", placeholder));
            tabs_->addTab(placeholder, "En espera");
        }
    }

    if (first && tabs_->count() == 0) {
        openTab();
    }

    if (tabs_->count() > 0) {
        tabs_->setCurrentIndex(0);
    }
    statusBar()->showMessage("Pestañas restauradas con éxito", 3000);
}

void BrowserWindow::handleCrashRecoveryDiscard() {
    if (crashRecoveryService_) {
        crashRecoveryService_->discardRecovery();
    }
    if (crashRecoveryBanner_) {
        crashRecoveryBanner_->hide();
    }
    if (tabs_->count() == 0) {
        openTab();
    }
    if (address_) {
        address_->setFocus();
    }
    statusBar()->showMessage("Sesión anterior descartada", 3000);
}

void BrowserWindow::handleCrashRecoveryDismiss() {
    if (crashRecoveryService_) {
        crashRecoveryService_->cancelRecovery();
    }
    if (crashRecoveryBanner_) {
        crashRecoveryBanner_->hide();
    }
    if (address_) {
        address_->setFocus();
    }
}

QWidget* BrowserWindow::newHomePage() {
    auto* page = new QWidget();
    page->setObjectName("vigilio-home");
    auto* layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    auto* logo = new QLabel(page);
    logo->setObjectName("vigilio-home-logo");
    logo->setAlignment(Qt::AlignCenter);
    const QIcon logoIcon = makeResourceIcon("branding/vigilio-logo.svg");
    logo->setPixmap(logoIcon.pixmap(64, 64));
    logo->setAccessibleName("Logotipo de Vigilio Browser");

    auto* title = new QLabel("Vigilio Browser", page);
    title->setObjectName("vigilio-home-title");
    title->setAlignment(Qt::AlignCenter);
    auto* tagline = new QLabel("Navega. Organiza. Protege.", page);
    tagline->setAlignment(Qt::AlignCenter);
    auto* input = new QLineEdit(page);
    input->setObjectName("vigilio-home-search");
    input->setPlaceholderText("Buscar en la web o introducir una dirección");
    input->setMinimumWidth(360);
    input->setAccessibleName("Buscar desde la nueva pestaña");
    connect(input, &QLineEdit::returnPressed, this,
            [this, input]() { navigateFromInput(input->text()); });
    layout->addWidget(logo);
    layout->addSpacing(12);
    layout->addWidget(title);
    layout->addWidget(tagline);
    layout->addSpacing(22);
    layout->addWidget(input);
    return page;
}
QWidget* BrowserWindow::createErrorPageWidget(const QUrl& failedUrl, QWebEngineView* view) {
    auto* page = new QWidget(tabs_);
    page->setObjectName("vigilio-page-error");
    page->setProperty("failedUrl", failedUrl);

    auto* layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(senior_ ? 24 : 16);

    auto* iconLabel = new QLabel(page);
    iconLabel->setObjectName("vigilio-error-icon");
    iconLabel->setAlignment(Qt::AlignCenter);
    const QIcon errorIcon = makeResourceIcon("icons/tools/shield.svg");
    iconLabel->setPixmap(errorIcon.pixmap(48, 48));
    iconLabel->setAccessibleName("Icono de advertencia de error al cargar la página");

    auto* title = new QLabel("No se pudo cargar la página", page);
    title->setObjectName("vigilio-error-title");
    title->setAlignment(Qt::AlignCenter);
    title->setAccessibleName("No se pudo cargar la página");
    QFont titleFont = title->font();
    titleFont.setPointSize(senior_ ? 24 : 20);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);

    auto* message = new QLabel("Comprueba la dirección web o la conexión del equipo.", page);
    message->setObjectName("vigilio-error-message");
    message->setAlignment(Qt::AlignCenter);
    message->setAccessibleName("Comprueba la dirección web o la conexión del equipo.");
    QFont msgFont = message->font();
    msgFont.setPointSize(senior_ ? 14 : 11);
    message->setFont(msgFont);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setAlignment(Qt::AlignCenter);
    btnLayout->setSpacing(senior_ ? 16 : 12);

    auto* retry = new QPushButton("Reintentar", page);
    retry->setObjectName("vigilio-error-retry");
    retry->setAccessibleName("Reintentar la carga de la página");
    retry->setMinimumHeight(senior_ ? 48 : 32);
    retry->setMinimumWidth(senior_ ? 160 : 120);
    retry->setDefault(true);

    auto* home = new QPushButton("Ir al inicio", page);
    home->setObjectName("vigilio-error-home");
    home->setAccessibleName("Volver a la página de inicio");
    home->setMinimumHeight(senior_ ? 48 : 32);
    home->setMinimumWidth(senior_ ? 160 : 120);

    QPointer<QWebEngineView> viewPtr = view;
    connect(retry, &QPushButton::clicked, this, [this, page, failedUrl, viewPtr]() {
        const int index = tabs_->indexOf(page);
        if (index >= 0) {
            {
                QSignalBlocker blocker(tabs_);
                tabs_->removeTab(index);
            }
            if (viewPtr) {
                tabs_->insertTab(index, viewPtr.data(), "Cargando…");
                tabs_->setCurrentIndex(index);
                viewPtr->reload();
            } else {
                openTab(failedUrl);
                const int latest = tabs_->count() - 1;
                if (index < latest) {
                    tabs_->tabBar()->moveTab(latest, index);
                }
                tabs_->setCurrentIndex(index);
            }
        }
        page->deleteLater();
    });

    connect(home, &QPushButton::clicked, this, [this, page, viewPtr]() {
        const int index = tabs_->indexOf(page);
        if (index >= 0) {
            {
                QSignalBlocker blocker(tabs_);
                tabs_->removeTab(index);
            }
            tabs_->insertTab(index, newHomePage(), startTitle);
            tabs_->setCurrentIndex(index);
        }
        if (viewPtr) {
            viewPtr->deleteLater();
        }
        page->deleteLater();
    });

    connect(page, &QObject::destroyed, this, [this, viewPtr]() {
        if (viewPtr && tabs_ && tabs_->indexOf(viewPtr.data()) < 0) {
            viewPtr->deleteLater();
        }
    });

    btnLayout->addWidget(retry);
    btnLayout->addWidget(home);

    layout->addWidget(iconLabel);
    layout->addWidget(title);
    layout->addWidget(message);
    layout->addSpacing(senior_ ? 20 : 12);
    layout->addLayout(btnLayout);

    retry->setFocus();
    return page;
}
void BrowserWindow::openTab(const QUrl& url) {
    if (url.isEmpty()) {
        const auto index = tabs_->addTab(newHomePage(), startTitle);
        tabs_->setCurrentIndex(index);
        return;
    }
    auto* view = new QWebEngineView(tabs_);
    const int index = tabs_->addTab(view, "Cargando…");
    tabs_->setCurrentIndex(index);
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString& title) {
        int i = tabs_->indexOf(view);
        if (i >= 0) {
            tabs_->setTabText(i, title.left(30));
        }
    });
    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl&) {
        if (tabs_->currentWidget() == view) {
            refreshAddressBar();
        }
    });
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (ok) {
            if (db_ && db_->valid() &&
                (view->url().scheme() == "https" || view->url().scheme() == "http")) {
                db_->recordVisit(view->url().toString().toStdString(), view->title().toStdString());
            }
            if (tabs_->currentWidget() == view) {
                refreshAddressBar();
                statusBar()->showMessage("Página cargada", 3000);
            }
            int i = tabs_->indexOf(view);
            if (i >= 0) {
                const QString t = view->title().trimmed();
                tabs_->setTabText(i, t.isEmpty() ? view->url().host().left(30) : t.left(30));
            }
        } else {
            int i = tabs_->indexOf(view);
            if (i >= 0) {
                auto* errorWidget = createErrorPageWidget(view->url(), view);
                {
                    QSignalBlocker blocker(tabs_);
                    tabs_->removeTab(i);
                    tabs_->insertTab(i, errorWidget, "No se pudo cargar");
                    tabs_->setCurrentIndex(i);
                }
                refreshAddressBar();
            }
            statusBar()->showMessage("No se pudo cargar la página", 3000);
        }
    });
    connect(view->page(), &QWebEnginePage::renderProcessTerminated, this,
            [this, view](QWebEnginePage::RenderProcessTerminationStatus, int) {
                int i = tabs_->indexOf(view);
                if (i >= 0) {
                    auto* errorWidget = createErrorPageWidget(view->url(), view);
                    {
                        QSignalBlocker blocker(tabs_);
                        tabs_->removeTab(i);
                        tabs_->insertTab(i, errorWidget, "No se pudo cargar");
                        tabs_->setCurrentIndex(i);
                    }
                    refreshAddressBar();
                }
                statusBar()->showMessage("No se pudo cargar la página", 3000);
            });
    view->load(url);
}
int BrowserWindow::tabCount() const {
    return tabs_->count();
}
QWebEngineView* BrowserWindow::currentWebView() const {
    return qobject_cast<QWebEngineView*>(tabs_->currentWidget());
}
void BrowserWindow::switchToWebView(const QUrl& url) {
    if (currentWebView()) {
        currentWebView()->load(url);
        return;
    }
    const auto index = tabs_->currentIndex();
    if (index >= 0) {
        auto* old = tabs_->widget(index);
        {
            QSignalBlocker blocker(tabs_);
            tabs_->removeTab(index);
            old->deleteLater();
        }
        openTab(url);
        const int latest = tabs_->count() - 1;
        if (index < latest) {
            tabs_->tabBar()->moveTab(latest, index);
        }
        tabs_->setCurrentIndex(index);
    } else {
        openTab(url);
    }
}
void BrowserWindow::navigate(const QUrl& url) {
    switchToWebView(url);
}
void BrowserWindow::navigateFromInput(const QString& value) {
    const auto target = vigilio::resolveInput(value.toStdString());
    if (target.kind == vigilio::NavigationTarget::Kind::Rejected) {
        QMessageBox::warning(this, "Dirección no permitida",
                             "Introduce una dirección HTTP/HTTPS o una búsqueda válida.");
        return;
    }
    navigate(QUrl(QString::fromStdString(target.value)));
}
void BrowserWindow::refreshAddressBar() {
    if (auto* view = currentWebView()) {
        address_->setText(view->url().toString());
    } else if (auto* current = tabs_->currentWidget()) {
        const auto failed = current->property("failedUrl");
        if (failed.isValid() && !failed.toUrl().isEmpty()) {
            address_->setText(failed.toUrl().toString());
        } else {
            address_->clear();
        }
    } else {
        address_->clear();
    }
}
void BrowserWindow::closeTab(int index) {
    if (index < 0 || index >= tabs_->count()) {
        return;
    }
    auto* widget = tabs_->widget(index);
    if (widget->property("pinned").toBool()) {
        statusBar()->showMessage("La pestaña fijada debe soltarse antes de cerrarla", 3000);
        return;
    }
    tabs_->removeTab(index);
    widget->deleteLater();
    if (tabs_->count() == 0) {
        openTab();
    }
}
void BrowserWindow::makeTabMenu(const QPoint& point) {
    const auto index = tabs_->tabBar()->tabAt(point);
    if (index < 0) {
        return;
    }
    auto* widget = tabs_->widget(index);
    QMenu menu(this);
    auto* newRight = menu.addAction("Nueva pestaña a la derecha");
    auto* duplicate = menu.addAction("Duplicar pestaña");
    auto* pin =
        menu.addAction(widget->property("pinned").toBool() ? "Desfijar pestaña" : "Fijar pestaña");
    auto* mute =
        menu.addAction(widget->property("muted").toBool() ? "Activar sonido" : "Silenciar pestaña");
    menu.addSeparator();
    auto* close = menu.addAction("Cerrar pestaña");
    auto* closeOthers = menu.addAction("Cerrar las demás pestañas");
    auto* picked = menu.exec(tabs_->tabBar()->mapToGlobal(point));
    if (!picked) {
        return;
    }
    if (picked == newRight) {
        openTab();
        tabs_->tabBar()->moveTab(tabs_->currentIndex(), index + 1);
    } else if (picked == duplicate) {
        if (auto* web = qobject_cast<QWebEngineView*>(widget)) {
            openTab(web->url());
        } else {
            openTab();
        }
    } else if (picked == pin) {
        widget->setProperty("pinned", !widget->property("pinned").toBool());
    } else if (picked == mute) {
        const bool target = !widget->property("muted").toBool();
        widget->setProperty("muted", target);
        if (auto* web = qobject_cast<QWebEngineView*>(widget)) {
            web->page()->setAudioMuted(target);
        }
    } else if (picked == close) {
        closeTab(index);
    } else if (picked == closeOthers) {
        for (int i = tabs_->count() - 1; i >= 0; --i) {
            if (tabs_->widget(i) != widget) {
                closeTab(i);
            }
        }
    }
}
void BrowserWindow::saveBookmark() {
    auto* view = currentWebView();
    if (!view || !db_ || !db_->valid()) {
        return;
    }
    const bool ok = db_->addBookmark(view->url().toString().toStdString(), view->title().toStdString());
    statusBar()->showMessage(ok ? "Favorito guardado localmente" : "No se pudo guardar el favorito",
                             3000);
}
void BrowserWindow::showBookmarks() {
    if (!db_ || !db_->valid()) {
        return;
    }
    QMenu menu(this);
    const auto items = db_->bookmarks();
    if (items.empty()) {
        menu.addAction("Sin favoritos")->setEnabled(false);
    }
    for (const auto& b : items) {
        auto* action = menu.addAction(QString::fromStdString(b.title));
        connect(action, &QAction::triggered, this,
                [this, b]() { openTab(QUrl(QString::fromStdString(b.url))); });
    }
    menu.exec(QCursor::pos());
}
void BrowserWindow::exportSettings() {
    const QString output = QFileDialog::getSaveFileName(
        this, "Exportar datos locales", QDir::homePath() + "/vigilio-settings.json", "JSON (*.json)");
    if (output.isEmpty()) {
        return;
    }
    QJsonObject obj;
    obj["format"] = "vigilio-settings-v1";
    obj["themeDark"] = dark_;
    obj["seniorMode"] = senior_;
    QJsonArray bookmarks;
    if (db_ && db_->valid()) {
        for (const auto& b : db_->bookmarks()) {
            QJsonObject bm;
            bm["url"] = QString::fromStdString(b.url);
            bm["title"] = QString::fromStdString(b.title);
            bookmarks.append(bm);
        }
    }
    obj["bookmarks"] = bookmarks;
    QSaveFile file(output);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(obj).toJson()) < 0 ||
        !file.commit()) {
        QMessageBox::critical(this, "Error de exportación",
                              "No se pudo guardar el archivo. No se han borrado tus datos locales.");
        return;
    }
    QMessageBox::information(this, "Datos exportados",
                             "Se guardaron ajustes y favoritos en un archivo local. No se incluyen "
                             "contraseñas, cookies ni historial.");
}
void BrowserWindow::toggleSeniorMode() {
    setSeniorMode(!senior_);
}
void BrowserWindow::setSeniorMode(bool enabled) {
    senior_ = enabled;
    seniorAction_->setChecked(enabled);
    QSettings().setValue("accessibility/senior", enabled);
    if (toolbar_) {
        toolbar_->setIconSize(enabled ? QSize(32, 32) : QSize(22, 22));
        toolbar_->setMinimumHeight(enabled ? 55 : 40);
    }
    QFont font = qApp->font();
    font.setPointSize(enabled ? 14 : 10);
    qApp->setFont(font);
    // All visible controls inherit a larger font and touch target when 'Interfaz más fácil de usar' is
    // active.
    for (auto* child : findChildren<QLineEdit*>()) {
        child->setMinimumHeight(enabled ? 48 : 32);
    }
    for (auto* child : findChildren<QPushButton*>()) {
        child->setMinimumHeight(enabled ? 48 : 32);
    }
    for (auto* child : findChildren<QToolBar*>()) {
        child->setMinimumHeight(enabled ? 55 : 40);
    }
    for (auto* dialog : findChildren<QDialog*>()) {
        for (auto* btn : dialog->findChildren<QPushButton*>()) {
            btn->setMinimumHeight(enabled ? 48 : 32);
            btn->setMinimumWidth(enabled ? 120 : 80);
            QFont f = btn->font();
            f.setPointSize(enabled ? 14 : 10);
            btn->setFont(f);
        }
    }

    if (safeModeBanner_) {
        safeModeBanner_->setMinimumHeight(enabled ? 55 : 42); // safeModeBanner_ minimumHeight scaling
        if (auto* dismiss = safeModeBanner_->findChild<QPushButton*>("vigilio-safe-mode-dismiss")) {
            dismiss->setMinimumHeight(enabled ? 48 : 32);
        }
        if (auto* iconLabel = safeModeBanner_->findChild<QLabel*>("vigilio-safe-mode-icon")) {
            iconLabel->setPixmap(
                makeResourceIcon("icons/tools/shield.svg").pixmap(enabled ? 28 : 20, enabled ? 28 : 20));
        }
    }
    if (crashRecoveryBanner_) {
        crashRecoveryBanner_->setMinimumHeight(enabled ? 56 : 42);
        if (auto* restoreBtn =
                crashRecoveryBanner_->findChild<QPushButton*>("vigilio-crash-recovery-restore")) {
            restoreBtn->setMinimumHeight(enabled ? 48 : 32);
        }
        if (auto* discardBtn =
                crashRecoveryBanner_->findChild<QPushButton*>("vigilio-crash-recovery-discard")) {
            discardBtn->setMinimumHeight(enabled ? 48 : 32);
        }
        if (auto* dismissBtn =
                crashRecoveryBanner_->findChild<QPushButton*>("vigilio-crash-recovery-dismiss")) {
            dismissBtn->setMinimumHeight(enabled ? 48 : 32);
        }
        if (auto* iconLabel = crashRecoveryBanner_->findChild<QLabel*>("vigilio-crash-recovery-icon")) {
            iconLabel->setPixmap(
                makeResourceIcon("icons/tools/shield.svg").pixmap(enabled ? 28 : 20, enabled ? 28 : 20));
        }
    }
    if (incognitoBadge_) {
        QFont f = incognitoBadge_->font();
        f.setPointSize(enabled ? 14 : 10);
        incognitoBadge_->setFont(f);
    }

    statusBar()->showMessage(enabled ? "Interfaz fácil activada" : "Interfaz fácil desactivada", 3000);
}
void BrowserWindow::setDarkTheme(bool enabled) {
    dark_ = enabled;
    themeAction_->setChecked(enabled);
    QSettings().setValue("appearance/dark", enabled);
    qApp->setStyleSheet(
        enabled
            ? "QMainWindow,QWidget{background:#141D2B;color:#E9EDF5;}"
              "QLineEdit,QTabWidget::pane{background:#222D3E;color:#FFFFFF;border:"
              "1px solid #394557;border-radius:7px;}"
              "QTabBar::tab{background:#1C2839;color:#E8ECF3;padding:8px 12px;}"
              "QTabBar::tab:selected{background:#33435A;}"
              "QToolBar{background:#182334;spacing:8px;border:0;}"
              "QMenu{background:#1A2637;color:#FFFFFF;border:1px solid #40516A;}"
              "QMenu::item:selected{background:#294F89;}"
              "QLabel#vigilio-home-title{font-size:30px;font-weight:600;}"
              "QWidget#vigilio-page-error{background:#141D2B;color:#E9EDF5;}"
              "QLabel#vigilio-error-title{color:#E9EDF5;font-size:20px;font-weight:600;}"
              "QLabel#vigilio-error-message{color:#94A3B8;font-size:14px;}"
              "QPushButton#vigilio-error-retry{background:#294F89;color:#FFFFFF;border:"
              "none;border-radius:6px;padding:8px 20px;font-weight:500;}"
              "QPushButton#vigilio-error-retry:hover{background:#3B82F6;}"
              "QPushButton#vigilio-error-home{background:#222D3E;color:#FFFFFF;border:"
              "1px solid #394557;border-radius:6px;padding:8px 20px;}"
              "QPushButton#vigilio-error-home:hover{background:#2D3B50;}"
              "QFrame#vigilio-safe-mode-banner{background:#1E293B;color:#E9EDF5;border-bottom:1px "
              "solid #334155;}"
              "QLabel#vigilio-safe-mode-label{color:#E9EDF5;font-size:13px;}"
              "QPushButton#vigilio-safe-mode-dismiss{background:#334155;color:#FFFFFF;border:none;"
              "border-radius:4px;padding:4px 12px;}"
              "QPushButton#vigilio-safe-mode-dismiss:hover{background:#475569;}"
              "QFrame#vigilio-crash-recovery-banner{background:#1E293B;color:#E9EDF5;border-bottom:1px "
              "solid #334155;}"
              "QLabel#vigilio-crash-recovery-label{color:#E9EDF5;font-size:13px;}"
              "QPushButton#vigilio-crash-recovery-restore{background:#2563EB;color:#FFFFFF;border:none;"
              "border-radius:4px;padding:4px 14px;font-weight:500;}"
              "QPushButton#vigilio-crash-recovery-restore:hover{background:#1D4ED8;}"
              "QPushButton#vigilio-crash-recovery-discard{background:#334155;color:#E2E8F0;border:none;"
              "border-radius:4px;padding:4px 12px;}"
              "QPushButton#vigilio-crash-recovery-discard:hover{background:#475569;}"
              "QPushButton#vigilio-crash-recovery-dismiss{background:transparent;color:#94A3B8;border:"
              "none;"
              "border-radius:4px;padding:4px 10px;}"
              "QPushButton#vigilio-crash-recovery-dismiss:hover{background:#334155;color:#FFFFFF;}"
              "QLabel#vigilio-incognito-badge{color:#94A3B8;padding:2px 8px;font-weight:500;}"
              "QDialog#vigilio-confirm-close-dialog{background:#1E293B;color:#E9EDF5;}"
              "QLabel#vigilio-confirm-close-title{color:#E9EDF5;font-weight:600;}"
              "QLabel#vigilio-confirm-close-desc{color:#94A3B8;}"
              "QPushButton#vigilio-confirm-close-cancel{background:#334155;color:#FFFFFF;border:none;"
              "border-radius:6px;padding:8px 16px;}"
              "QPushButton#vigilio-confirm-close-cancel:hover{background:#475569;}"
              "QPushButton#vigilio-confirm-close-confirm{background:#294F89;color:#FFFFFF;border:none;"
              "border-radius:6px;padding:8px 20px;font-weight:500;}"
              "QPushButton#vigilio-confirm-close-confirm:hover{background:#3B82F6;}"
            : "QMainWindow,QWidget{background:#F7F9FC;color:#192538;}"
              "QLineEdit,QTabWidget::pane{background:#FFFFFF;color:#192538;border:"
              "1px solid #D9E0EA;border-radius:7px;}"
              "QTabBar::tab{background:#E9EEF6;color:#192538;padding:8px 12px;}"
              "QTabBar::tab:selected{background:#FFFFFF;}"
              "QToolBar{background:#F3F6FB;spacing:8px;border:0;}"
              "QMenu{background:#FFFFFF;color:#192538;border:1px solid #D9E0EA;}"
              "QMenu::item:selected{background:#DCE9FA;}"
              "QLabel#vigilio-home-title{font-size:30px;font-weight:600;}"
              "QWidget#vigilio-page-error{background:#F7F9FC;color:#192538;}"
              "QLabel#vigilio-error-title{color:#192538;font-size:20px;font-weight:600;}"
              "QLabel#vigilio-error-message{color:#5A6A80;font-size:14px;}"
              "QPushButton#vigilio-error-retry{background:#2563EB;color:#FFFFFF;border:"
              "none;border-radius:6px;padding:8px 20px;font-weight:500;}"
              "QPushButton#vigilio-error-retry:hover{background:#1D4ED8;}"
              "QPushButton#vigilio-error-home{background:#FFFFFF;color:#192538;border:"
              "1px solid #D9E0EA;border-radius:6px;padding:8px 20px;}"
              "QPushButton#vigilio-error-home:hover{background:#F1F4F9;}"
              "QFrame#vigilio-safe-mode-banner{background:#EFF6FF;color:#1E3A8A;border-bottom:1px "
              "solid #BFDBFE;}"
              "QLabel#vigilio-safe-mode-label{color:#1E3A8A;font-size:13px;}"
              "QPushButton#vigilio-safe-mode-dismiss{background:#DBEAFE;color:#1E40AF;border:none;"
              "border-radius:4px;padding:4px 12px;}"
              "QPushButton#vigilio-safe-mode-dismiss:hover{background:#BFDBFE;}"
              "QFrame#vigilio-crash-recovery-banner{background:#EFF6FF;color:#1E3A8A;border-bottom:1px "
              "solid #BFDBFE;}"
              "QLabel#vigilio-crash-recovery-label{color:#1E3A8A;font-size:13px;}"
              "QPushButton#vigilio-crash-recovery-restore{background:#2563EB;color:#FFFFFF;border:none;"
              "border-radius:4px;padding:4px 14px;font-weight:500;}"
              "QPushButton#vigilio-crash-recovery-restore:hover{background:#1D4ED8;}"
              "QPushButton#vigilio-crash-recovery-discard{background:#DBEAFE;color:#1E40AF;border:none;"
              "border-radius:4px;padding:4px 12px;}"
              "QPushButton#vigilio-crash-recovery-discard:hover{background:#BFDBFE;}"
              "QPushButton#vigilio-crash-recovery-dismiss{background:transparent;color:#64748B;border:"
              "none;"
              "border-radius:4px;padding:4px 10px;}"
              "QPushButton#vigilio-crash-recovery-dismiss:hover{background:#E2E8F0;color:#0F172A;}"
              "QLabel#vigilio-incognito-badge{color:#475569;padding:2px 8px;font-weight:500;}"
              "QDialog#vigilio-confirm-close-dialog{background:#FFFFFF;color:#192538;}"
              "QLabel#vigilio-confirm-close-title{color:#192538;font-weight:600;}"
              "QLabel#vigilio-confirm-close-desc{color:#5A6A80;}"
              "QPushButton#vigilio-confirm-close-cancel{background:#E9EEF6;color:#192538;border:1px "
              "solid #D9E0EA;border-radius:6px;padding:8px 16px;}"
              "QPushButton#vigilio-confirm-close-cancel:hover{background:#DCE9FA;}"
              "QPushButton#vigilio-confirm-close-confirm{background:#2563EB;color:#FFFFFF;border:none;"
              "border-radius:6px;padding:8px 20px;font-weight:500;}"
              "QPushButton#vigilio-confirm-close-confirm:hover{background:#1D4ED8;}");
}
void BrowserWindow::saveSession() {
    if (isIncognito_) {
        return;
    }
    QStringList urls;
    for (int i = 0; i < tabs_->count(); ++i) {
        const auto* widget = tabs_->widget(i);
        if (auto* v = qobject_cast<QWebEngineView*>(widget)) {
            if (v->url().scheme() == "http" || v->url().scheme() == "https") {
                urls << v->url().toString();
            }
        } else if (widget->property("deferredUrl").isValid()) {
            urls << widget->property("deferredUrl").toString();
        }
    }
    QSettings().setValue("session/urls", urls);
}
void BrowserWindow::restoreSession() {
    const auto urls = QSettings().value("session/urls").toStringList();
    // Only the first page loads. Other tabs are placeholders and consume no WebEngine instance.
    bool first = true;
    for (const auto& url : urls.mid(0, 100)) {
        if (QUrl(url).scheme() != "http" && QUrl(url).scheme() != "https") {
            continue;
        }
        if (first) {
            openTab(QUrl(url));
            first = false;
            continue;
        }
        auto* placeholder = new QWidget(tabs_);
        placeholder->setProperty("deferredUrl", url);
        auto* layout = new QVBoxLayout(placeholder);
        layout->addWidget(new QLabel("Pestaña en espera. Se cargará al abrirla.", placeholder));
        tabs_->addTab(placeholder, "En espera");
    }
    if (tabs_->count() > 0) {
        tabs_->setCurrentIndex(0);
    }
}
void BrowserWindow::activateDeferredTab(int index) {
    if (index < 0 || index >= tabs_->count() || tabs_->currentIndex() != index) {
        return;
    }
    auto* old = tabs_->widget(index);
    const auto value = old->property("deferredUrl");
    if (!value.isValid()) {
        return;
    }
    const QUrl target(value.toString());
    {
        QSignalBlocker blocker(tabs_);
        tabs_->removeTab(index);
        old->deleteLater();
    }
    // openTab wires all events, then return the view to its original position.
    openTab(target);
    const int latest = tabs_->count() - 1;
    if (index < latest) {
        tabs_->tabBar()->moveTab(latest, index);
    }
    tabs_->setCurrentIndex(index);
}
bool BrowserWindow::confirmCloseMultipleTabs() {
    QDialog dialog(this);
    dialog.setObjectName("vigilio-confirm-close-dialog");
    dialog.setWindowTitle("Cerrar ventana");
    dialog.setAccessibleName("Confirmación de cierre de múltiples pestañas");
    dialog.setModal(true);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(senior_ ? 20 : 12);
    layout->setContentsMargins(senior_ ? 24 : 16, senior_ ? 24 : 16, senior_ ? 24 : 16,
                               senior_ ? 24 : 16);

    auto* titleLabel =
        new QLabel(QString("¿Quieres cerrar las %1 pestañas abiertas?").arg(tabs_->count()), &dialog);
    titleLabel->setObjectName("vigilio-confirm-close-title");
    titleLabel->setAccessibleName(titleLabel->text());
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(senior_ ? 16 : 12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto* descLabel =
        new QLabel("Puedes volver a abrirlas la próxima vez que inicies el navegador.", &dialog);
    descLabel->setObjectName("vigilio-confirm-close-desc");
    QFont descFont = descLabel->font();
    descFont.setPointSize(senior_ ? 14 : 10);
    descLabel->setFont(descFont);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(senior_ ? 16 : 10);
    btnLayout->addStretch(1);

    auto* cancelBtn = new QPushButton("Cancelar", &dialog);
    cancelBtn->setObjectName("vigilio-confirm-close-cancel");
    cancelBtn->setAccessibleName("Cancelar y permanecer en el navegador");
    cancelBtn->setMinimumHeight(senior_ ? 48 : 32);
    cancelBtn->setMinimumWidth(senior_ ? 120 : 80);

    auto* closeBtn = new QPushButton("Cerrar todas", &dialog);
    closeBtn->setObjectName("vigilio-confirm-close-confirm");
    closeBtn->setAccessibleName("Cerrar todas las pestañas y salir");
    closeBtn->setMinimumHeight(senior_ ? 48 : 32);
    closeBtn->setMinimumWidth(senior_ ? 120 : 90);
    closeBtn->setDefault(true);

    if (senior_) {
        QFont btnFont = cancelBtn->font();
        btnFont.setPointSize(14);
        cancelBtn->setFont(btnFont);
        closeBtn->setFont(btnFont);
    }

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(closeBtn);

    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    closeBtn->setFocus();

    return dialog.exec() == QDialog::Accepted;
}

void BrowserWindow::closeEvent(QCloseEvent* event) {
    if (lifetimeCoordinator_) {
        const auto action =
            lifetimeCoordinator_->requestShutdown(vigilio::ShutdownReason::UserWindowClose);
        if (action == vigilio::ShutdownAction::IgnoreRequest) {
            event->ignore();
            return;
        }
    }

    QSettings settings;
    const bool warnOnClose = settings.value("browser/warnOnCloseMultipleTabs", true).toBool();
    if (tabs_->count() > 1 && !isIncognito_ && warnOnClose) {
        if (!confirmCloseMultipleTabs()) {
            if (lifetimeCoordinator_) {
                lifetimeCoordinator_->transitionTo(vigilio::AppLifetimeState::Running);
            }
            event->ignore();
            return;
        }
    }

    if (heartbeatTimer_) {
        heartbeatTimer_->stop();
    }

    if (lifetimeCoordinator_) {
        const auto report = lifetimeCoordinator_->executeShutdown(
            vigilio::ShutdownReason::UserWindowClose,
            [this]() -> bool {
                saveSession();
                return true;
            },
            [this]() -> bool {
                if (db_) {
                    db_->close();
                }
                return true;
            });
        if (crashRecoveryService_ && report.cleanExit) {
            crashRecoveryService_->finalizeShutdown(true, vigilio::CrashReason::CleanShutdown);
        }
    } else {
        saveSession();
        if (db_) {
            db_->close();
        }
        if (crashRecoveryService_) {
            crashRecoveryService_->finalizeShutdown(true, vigilio::CrashReason::CleanShutdown);
        }
    }

    event->accept();
    QMainWindow::closeEvent(event);
}
