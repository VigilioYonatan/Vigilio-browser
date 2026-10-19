#pragma once

#include "core/MainWindowLayoutTypes.h"
#include "core/TitleBarConfiguration.h"
#include "core/TitleBarTypes.h"
#include "core/TitleBarValidator.h"

#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace vigilio {

/// @brief Opciones de configuración operativa para el servicio de barra de título.
struct TitleBarServiceOptions {
    bool autoValidateOnMutation{true};
    bool enablePersistence{true};
    bool isIncognitoSession{false};
    std::filesystem::path stateFilePath{};
};

using OnTitleBarChangedCallback = std::function<void(const TitleBarLayoutPlan&, const TitleBarReport&)>;

/// @brief Servicio operativo propietario desacoplado para la gestión de la barra de título integrada.
class TitleBarService {
  public:
    TitleBarService();
    explicit TitleBarService(TitleBarConfig initialConfig, int windowWidth = 1200,
                             TitleBarServiceOptions options = {});
    explicit TitleBarService(LayoutDensity density, int windowWidth = 1200,
                             TitleBarServiceOptions options = {});
    explicit TitleBarService(TitleBarLayoutPlan initialPlan, TitleBarServiceOptions options = {});
    ~TitleBarService() = default;

    TitleBarService(const TitleBarService&) = delete;
    TitleBarService& operator=(const TitleBarService&) = delete;
    TitleBarService(TitleBarService&& other) noexcept;
    TitleBarService& operator=(TitleBarService&& other) noexcept;

    // Acceso thread-safe a configuración, plan y opciones
    [[nodiscard]] TitleBarConfig config() const;
    [[nodiscard]] TitleBarLayoutPlan plan() const;
    [[nodiscard]] int windowWidth() const;
    [[nodiscard]] TitleBarServiceOptions options() const;
    void setOptions(const TitleBarServiceOptions& options);

    // Conmutación de densidades visuales
    [[nodiscard]] TitleBarReport setDensity(LayoutDensity density);

    // Conmutación de modalidad de barra de título
    [[nodiscard]] TitleBarReport setMode(TitleBarMode mode);

    // Conmutación de colocación de controles
    [[nodiscard]] TitleBarReport setControlsPlacement(TitleBarControlsPlacement placement);

    // Título de la ventana
    [[nodiscard]] TitleBarReport setTitle(std::string_view title);

    // Visibilidad de componentes auxiliares
    [[nodiscard]] TitleBarReport setShowIcon(bool show);
    [[nodiscard]] TitleBarReport setShowSeniorToggle(bool show);

    // Estado de la ventana y botones
    [[nodiscard]] TitleBarReport setWindowState(const TitleBarState& state);
    [[nodiscard]] TitleBarReport updateButtonState(TitleBarButtonType buttonType,
                                                   TitleBarButtonState state);

    // Redimensionamiento de ventana
    [[nodiscard]] TitleBarReport setWindowWidth(int windowWidth);

    // Hit-testing cartesiano thread-safe
    [[nodiscard]] TitleBarHitTestZone hitTest(int x, int y) const noexcept;

    // Recálculo analítico con cancelación cooperativa
    [[nodiscard]] TitleBarReport computeLayout(CancellationToken* cancel = nullptr) const;

    // Persistencia local atómica y resiliencia
    [[nodiscard]] bool saveStateToFile(const std::filesystem::path& path,
                                       CancellationToken* cancel = nullptr);
    [[nodiscard]] bool loadStateFromFile(const std::filesystem::path& path,
                                         CancellationToken* cancel = nullptr);
    [[nodiscard]] bool saveCurrentState(CancellationToken* cancel = nullptr);
    [[nodiscard]] bool restoreDefault();

    // Privacidad y modo incógnito
    void setIncognitoMode(bool incognito) noexcept;
    [[nodiscard]] bool isIncognitoMode() const noexcept;

    // Observabilidad y notificaciones
    void setTitleBarChangedCallback(OnTitleBarChangedCallback callback);
    void clearTitleBarChangedCallback() noexcept;
    [[nodiscard]] static std::string formatTitleBarSummary(const TitleBarLayoutPlan& plan,
                                                           const TitleBarReport& report);

  private:
    TitleBarConfig config_{};
    TitleBarLayoutPlan plan_{};
    int windowWidth_{1200};
    TitleBarServiceOptions options_{};
    OnTitleBarChangedCallback callback_{nullptr};
    mutable std::mutex mutex_;

    void restoreDefaultLocked();
    void notifyChangedLocked(const TitleBarReport& report);
};

} // namespace vigilio
