#pragma once

#include "core/MainWindowLayoutConfiguration.h"
#include "core/MainWindowLayoutTypes.h"

#include <string>
#include <string_view>

namespace vigilio {

/// @brief Validador estático desacoplado, cálculo analítico de distribución espacial
///        y utilidades de textos accesibles en español neutro.
class MainWindowLayoutValidator {
  public:
    /// @brief Valida que las dimensiones de la ventana cumplan los límites mínimos según la densidad
    /// activa.
    [[nodiscard]] static MainWindowLayoutError validateDimensions(const WindowDimensions& dimensions,
                                                                  LayoutDensity density) noexcept;

    /// @brief Valida que la configuración de la barra lateral cumpla las invariantes del producto.
    [[nodiscard]] static MainWindowLayoutError validateSidebar(const SidebarConfig& sidebar) noexcept;

    /// @brief Realiza una auditoría estructural integral sobre un plan de layout de ventana.
    [[nodiscard]] static MainWindowLayoutError validatePlan(const MainWindowLayoutPlan& plan) noexcept;

    /// @brief Computa la distribución geométrica analítica de zonas para una ventana y determina el área
    /// neta de contenido.
    [[nodiscard]] static MainWindowLayoutReport calculateLayoutAreas(int windowWidth, int windowHeight,
                                                                     const MainWindowLayoutPlan& plan);

    /// @brief Convierte un error de layout en un mensaje explicativo en español neutro.
    [[nodiscard]] static std::string layoutErrorToString(MainWindowLayoutError error);

    /// @brief Convierte una zona de ventana en su etiqueta descriptiva en español neutro.
    [[nodiscard]] static std::string layoutZoneToString(LayoutZone zone);

    /// @brief Convierte una densidad de layout en su denominación pública oficial.
    [[nodiscard]] static std::string layoutDensityToString(LayoutDensity density);

    /// @brief Convierte el estado de la barra lateral en texto descriptivo en español neutro.
    [[nodiscard]] static std::string sidebarStateToString(SidebarState state);

    /// @brief Convierte la posición de acople de la barra lateral en texto en español neutro.
    [[nodiscard]] static std::string sidebarDockPositionToString(SidebarDockPosition position);

    /// @brief Convierte la modalidad de decoración de ventana en texto en español neutro.
    [[nodiscard]] static std::string chromeModeToString(WindowChromeMode mode);

    /// @brief Genera una sugerencia de remediación clara y accesible en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(MainWindowLayoutError error,
                                                             std::string_view details = {});
};

} // namespace vigilio
