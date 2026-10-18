#include "core/StartupBootstrapCoordinator.h"
#include "ui/BrowserWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QString>
#include <cstdlib>
#include <iostream>

namespace {
void showStartupErrorDialog(const vigilio::StartupDiagnosticReport& report) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Vigilio Browser");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("No se pudo iniciar Vigilio Browser");

    QString detail = "Ocurrió una dificultad al preparar el entorno del navegador.\n\n";
    if (report.status == vigilio::StartupStatusCode::StorageAccessDenied) {
        detail += "No se tienen permisos suficientes para acceder a la carpeta de perfil.";
    } else if (report.status == vigilio::StartupStatusCode::CorruptCriticalResources) {
        detail += "Algunos recursos esenciales del programa no pudieron cargarse.";
    } else {
        detail += "La inicialización del sistema no pudo completarse satisfactoriamente.";
    }

    if (!report.remediationAction.empty()) {
        detail += "\n\nSugerencia: " + QString::fromStdString(report.remediationAction);
    }

    msgBox.setInformativeText(detail);
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setDefaultButton(QMessageBox::Close);
    msgBox.exec();
}
} // namespace

int main(int argc, char** argv) {
    const auto cmdLine = vigilio::StartupBootstrapCoordinator::parseCommandLine(argc, argv);

    if (cmdLine.helpRequested) {
        std::cout << "Uso: vigilio-browser [opciones] [URL...]\n\n"
                  << "Opciones:\n"
                  << "  --headless              Ejecutar en modo sin ventana para automatización\n"
                  << "  --safe-mode             Iniciar en modo seguro a prueba de fallos\n"
                  << "  --incognito             Navegación privada con almacenamiento en memoria\n"
                  << "  --profile-dir <ruta>    Especificar directorio de perfil personalizado\n"
                  << "  -h, --help              Mostrar este mensaje de ayuda y salir\n"
                  << "  -v, --version           Mostrar versión de la aplicación y salir\n";
        return 0;
    }

    if (cmdLine.versionRequested) {
        std::cout << "Vigilio Browser 0.1.0-alpha\n";
        return 0;
    }

    if (!cmdLine.isValid) {
        std::cerr << "[Vigilio Startup Error] " << cmdLine.parseErrorMessage << "\n";
        for (const auto& rejected : cmdLine.rejectedArguments) {
            std::cerr << "  Argumento no permitido: " << rejected << "\n";
        }
        return 10;
    }

    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");

    const auto report =
        vigilio::StartupBootstrapCoordinator::executeBootstrap(cmdLine, display, wayland);

    if (!report.isSuccess()) {
        std::cerr << "[Vigilio Startup Error] " << report.diagnosticMessage << "\n";
        if (!report.remediationAction.empty()) {
            std::cerr << "[Remediation] " << report.remediationAction << "\n";
        }
        if (display || wayland) {
            QApplication errApp(argc, argv);
            showStartupErrorDialog(report);
        }
        return report.exitCode;
    }

    if (report.status == vigilio::StartupStatusCode::DegradedHeadless) {
        std::cerr << "[Vigilio Degraded] " << report.diagnosticMessage << "\n";
        if (!report.remediationAction.empty()) {
            std::cerr << "[Remediation] " << report.remediationAction << "\n";
        }
        return report.exitCode;
    }

    if (report.effectiveMode == vigilio::StartupMode::Headless) {
        std::cout << "[Vigilio Headless] " << report.diagnosticMessage << "\n";
        return 0;
    }

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Vigilio");
    QCoreApplication::setApplicationName("Vigilio Browser");
    QCoreApplication::setApplicationVersion("0.1.0-alpha");
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss}] [%{type}] %{category}: %{message}");
    BrowserWindow window(cmdLine, report);
    window.show();
    return app.exec();
}
