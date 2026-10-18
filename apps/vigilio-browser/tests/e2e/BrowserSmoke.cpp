// Optional, single-instance GUI smoke test (only built with Qt and VIGILIO_ENABLE_E2E=ON).
// Not a substitute for full navigation/security/accessibility E2E coverage.
#include "ui/BrowserWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <iostream>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VigilioTests");
    QCoreApplication::setApplicationName("E2E-Temporary-Profile");
    QTemporaryDir directory;
    if (!directory.isValid())
        return 1;
    const auto path = directory.filePath("fixture.html");
    QFile fixture(path);
    if (!fixture.open(QIODevice::WriteOnly))
        return 2;
    fixture.write("<!doctype html><title>Vigilio E2E Fixture</title><h1>ok</h1>");
    fixture.close();
    BrowserWindow window;
    window.show();
    const auto before = window.tabCount();
    window.openTab(QUrl::fromLocalFile(path));
    auto* view = window.currentWebView();
    if (!view || window.tabCount() != before + 1)
        return 3;
    QSignalSpy finished(view, &QWebEngineView::loadFinished);
    view->reload(); // Ensure the signal is observed after the spy is attached.
    if (finished.wait(20000) && view->title() == "Vigilio E2E Fixture") {
        std::cout << "GUI smoke PASS\n";
        return 0;
    }
    std::cerr << "GUI smoke FAIL: local fixture not loaded\n";
    return 4;
}
