#pragma once

#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutTypes.h"
#include "core/MainWindowLayoutValidator.h"

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>

namespace vigilio {

/// @brief Opciones de configuración operativa para el servicio de layout.
struct MainWindowLayoutServiceOptions {
    bool autoValidateOnMutation{true};
    bool allowNegativeContentArea{false}; // Siempre false por invariante
    bool enablePersistence{true};
    bool isIncognitoSession{false}; // Si es true, prohíbe persistencia en disco
    std::filesystem::path stateFilePath{};
};

using OnLayoutChangedCallback =
    std::function<void(const MainWindowLayoutPlan&, const MainWindowLayoutReport&)>;

/// @brief Servicio operativo propietario desacoplado para la gestión del layout de la ventana principal.
class MainWindowLayoutService {
  public:
    MainWindowLayoutService();
    explicit MainWindowLayoutService(MainWindowLayoutPlan initialPlan,
                                     MainWindowLayoutServiceOptions options = {});
    ~MainWindowLayoutService() = default;

    MainWindowLayoutService(const MainWindowLayoutService&) = delete;
    MainWindowLayoutService& operator=(const MainWindowLayoutService&) = delete;
    MainWindowLayoutService(MainWindowLayoutService&& other) noexcept;
    MainWindowLayoutService& operator=(MainWindowLayoutService&& other) noexcept;

    // Acceso thread-safe al plan y opciones
    [[nodiscard]] MainWindowLayoutPlan plan() const;
    [[nodiscard]] MainWindowLayoutServiceOptions options() const;
    void setOptions(const MainWindowLayoutServiceOptions& options);

    // Conmutación de densidades visuales
    [[nodiscard]] MainWindowLayoutReport setDensity(LayoutDensity density);

    // Dimensionamiento de ventana
    [[nodiscard]] MainWindowLayoutReport resizeWindow(int width, int height);

    // Control de barra lateral (Sidebar)
    [[nodiscard]] MainWindowLayoutReport setSidebarState(SidebarState state);
    [[nodiscard]] MainWindowLayoutReport toggleSidebar();
    [[nodiscard]] MainWindowLayoutReport setSidebarWidth(int width);
    [[nodiscard]] MainWindowLayoutReport setSidebarDockPosition(SidebarDockPosition position);

    // Control dinámico de zonas
    [[nodiscard]] MainWindowLayoutReport setZoneVisible(LayoutZone zone, bool visible);
    [[nodiscard]] MainWindowLayoutReport showNotificationBanner(bool show);
    [[nodiscard]] MainWindowLayoutReport showStatusBar(bool show);

    // Cálculo y evaluación geométrica
    [[nodiscard]] MainWindowLayoutReport computeLayout(CancellationToken* cancel = nullptr) const;

    // Persistencia y resiliencia local (Local-First)
    [[nodiscard]] bool saveLayoutStateToFile(const std::filesystem::path& path,
                                             CancellationToken* cancel = nullptr);
    [[nodiscard]] bool loadLayoutStateFromFile(const std::filesystem::path& path,
                                               CancellationToken* cancel = nullptr);
    [[nodiscard]] bool saveCurrentState(CancellationToken* cancel = nullptr);
    [[nodiscard]] bool restoreDefaultLayout();

    // Privacidad y modo incógnito
    void setIncognitoMode(bool incognito) noexcept;
    [[nodiscard]] bool isIncognitoMode() const noexcept;

    // Observabilidad y notificaciones
    void setLayoutChangedCallback(OnLayoutChangedCallback callback);
    void clearLayoutChangedCallback() noexcept;
    [[nodiscard]] static std::string formatLayoutSummary(const MainWindowLayoutPlan& plan,
                                                         const MainWindowLayoutReport& report);

  private:
    MainWindowLayoutPlan plan_;
    MainWindowLayoutServiceOptions options_;
    OnLayoutChangedCallback callback_{nullptr};
    mutable std::mutex mutex_;

    void notifyChangedLocked(const MainWindowLayoutReport& report);
};

} // namespace vigilio
