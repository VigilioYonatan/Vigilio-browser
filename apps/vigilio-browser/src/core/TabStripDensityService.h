#pragma once

#include "core/MainWindowLayoutTypes.h"
#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityTypes.h"
#include "core/TabStripDensityValidator.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Opciones de configuración operativa para el servicio de tira de pestañas.
struct TabStripDensityServiceOptions {
    bool autoValidateOnMutation{true};
    bool enablePersistence{true};
    bool isIncognitoSession{false};
    std::filesystem::path stateFilePath{};
};

using OnTabStripChangedCallback =
    std::function<void(const TabStripDensityPlan&, const TabStripDensityReport&)>;

/// @brief Servicio operativo propietario desacoplado para la gestión de la tira de pestañas.
class TabStripDensityService {
  public:
    TabStripDensityService();
    explicit TabStripDensityService(TabStripDensityConfig initialConfig, int availableWidth = 1000,
                                    std::vector<TabSpec> initialTabs = {},
                                    TabStripDensityServiceOptions options = {});
    explicit TabStripDensityService(LayoutDensity density, int availableWidth = 1000,
                                    TabStripDensityServiceOptions options = {});
    explicit TabStripDensityService(TabStripDensityPlan initialPlan,
                                    TabStripDensityServiceOptions options = {});
    ~TabStripDensityService() = default;

    TabStripDensityService(const TabStripDensityService&) = delete;
    TabStripDensityService& operator=(const TabStripDensityService&) = delete;
    TabStripDensityService(TabStripDensityService&& other) noexcept;
    TabStripDensityService& operator=(TabStripDensityService&& other) noexcept;

    // Acceso thread-safe a configuración, plan, pestañas y opciones
    [[nodiscard]] TabStripDensityConfig config() const;
    [[nodiscard]] TabStripDensityPlan plan() const;
    [[nodiscard]] int availableWidth() const;
    [[nodiscard]] std::vector<TabSpec> tabs() const;
    [[nodiscard]] std::size_t tabCount() const;
    [[nodiscard]] std::optional<TabSpec> tab(std::uint64_t tabId) const;
    [[nodiscard]] std::optional<TabSpec> tabAtIndex(std::size_t index) const;
    [[nodiscard]] std::optional<std::uint64_t> selectedTabId() const;
    [[nodiscard]] bool isOverflowing() const;
    [[nodiscard]] TabStripDensityServiceOptions options() const;
    void setOptions(const TabStripDensityServiceOptions& options);

    // Conmutación de densidades visuales y desbordamiento
    [[nodiscard]] TabStripDensityReport setDensity(LayoutDensity density);
    [[nodiscard]] TabStripDensityReport setOverflowMode(TabStripOverflowMode mode);
    [[nodiscard]] TabStripDensityReport setScrollOffset(int offset);
    [[nodiscard]] TabStripDensityReport setShowNewTabButton(bool show);

    // Mutaciones de la colección de pestañas
    [[nodiscard]] TabStripDensityReport addTab(const TabSpec& tab);
    [[nodiscard]] TabStripDensityReport insertTab(std::size_t index, const TabSpec& tab);
    [[nodiscard]] TabStripDensityReport removeTab(std::uint64_t tabId);
    [[nodiscard]] TabStripDensityReport moveTab(std::uint64_t tabId, std::size_t newIndex);
    [[nodiscard]] TabStripDensityReport selectTab(std::uint64_t tabId);
    [[nodiscard]] TabStripDensityReport setTabPinned(std::uint64_t tabId, bool pinned);
    [[nodiscard]] TabStripDensityReport setTabMediaState(std::uint64_t tabId, TabMediaState state);
    [[nodiscard]] TabStripDensityReport toggleTabAudioMute(std::uint64_t tabId);
    [[nodiscard]] TabStripDensityReport setTabDiscardState(std::uint64_t tabId, TabDiscardState state);
    [[nodiscard]] TabStripDensityReport updateTabTitle(std::uint64_t tabId, std::string_view title,
                                                       std::string_view url = {});

    // Redimensionamiento de ventana / tira
    [[nodiscard]] TabStripDensityReport setAvailableWidth(int availableWidth);

    // Hit-testing cartesiano e identificación de pestaña
    [[nodiscard]] TabStripHitTestZone hitTest(int x, int y) const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> tabAtPoint(int x, int y) const noexcept;

    // Recálculo analítico con cancelación cooperativa
    [[nodiscard]] TabStripDensityReport computeLayout(CancellationToken* cancel = nullptr) const;

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
    void setTabStripChangedCallback(OnTabStripChangedCallback callback);
    void clearTabStripChangedCallback() noexcept;
    [[nodiscard]] static std::string formatTabStripSummary(const TabStripDensityPlan& plan,
                                                           const TabStripDensityReport& report);

  private:
    TabStripDensityConfig config_{};
    int availableWidth_{1000};
    std::vector<TabSpec> tabs_{};
    TabStripDensityPlan plan_{};
    TabStripDensityServiceOptions options_{};
    OnTabStripChangedCallback callback_{nullptr};
    mutable std::mutex mutex_;

    void restoreDefaultLocked();
    void notifyChangedLocked(const TabStripDensityReport& report);
    [[nodiscard]] TabStripDensityReport recalculateLayoutLocked();
};

} // namespace vigilio
