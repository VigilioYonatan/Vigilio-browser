#pragma once

#include "core/MainWindowLayoutTypes.h"
#include "core/TabStripDensityConfiguration.h"
#include "core/TabStripDensityTypes.h"

#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Validador estático desacoplado, algoritmo determinista de distribución espacial,
///        hit-testing analítico y utilidades de accesibilidad y textos públicos en español neutro.
class TabStripDensityValidator {
  public:
    /// @brief Valida que las dimensiones cumplan con los límites mínimos según la densidad activa.
    [[nodiscard]] static TabStripDensityError validateDimensions(int width, int height,
                                                                 LayoutDensity density) noexcept;

    /// @brief Valida que el ancho cumpla con los límites mínimos según la densidad activa.
    [[nodiscard]] static TabStripDensityError validateDimensions(int width,
                                                                 LayoutDensity density) noexcept;

    /// @brief Realiza una auditoría estructural integral sobre un plan de tira de pestañas.
    [[nodiscard]] static TabStripDensityError validatePlan(const TabStripDensityPlan& plan) noexcept;

    /// @brief Computa la distribución geométrica analítica de la tira de pestañas con compresión
    /// elástica.
    [[nodiscard]] static TabStripDensityReport
    calculateTabStripLayout(int availableWidth, const TabStripDensityConfig& config,
                            const std::vector<TabSpec>& tabs);

    /// @brief Clasifica analíticamente un punto de coordenadas relativas (x, y) en su zona de impacto.
    [[nodiscard]] static TabStripHitTestZone hitTest(int x, int y,
                                                     const TabStripDensityPlan& plan) noexcept;

    /// @brief Convierte un error de tira de pestañas en un mensaje comprensible en español neutro.
    [[nodiscard]] static std::string tabStripErrorToString(TabStripDensityError error);

    /// @brief Convierte la densidad visual en su denominación en español neutro.
    [[nodiscard]] static std::string densityToString(LayoutDensity density);

    /// @brief Convierte una zona de impacto en su denominación descriptiva en español neutro.
    [[nodiscard]] static std::string hitTestZoneToString(TabStripHitTestZone zone);

    /// @brief Genera la etiqueta accesible para lectores de pantalla en español neutro.
    [[nodiscard]] static std::string tabAccessibleLabel(const TabSpec& tab, bool isSelected,
                                                        LayoutDensity density);

    /// @brief Genera la descripción flotante (tooltip) en español neutro.
    [[nodiscard]] static std::string tabTooltip(const TabSpec& tab, bool isSelected);

    /// @brief Genera la etiqueta accesible del botón de cierre en español neutro.
    [[nodiscard]] static std::string closeButtonAccessibleLabel(const TabSpec& tab);

    /// @brief Genera la etiqueta accesible del botón de control de audio en español neutro.
    [[nodiscard]] static std::string audioButtonAccessibleLabel(bool isMuted);

    /// @brief Genera la etiqueta accesible del botón de nueva pestaña en español neutro.
    [[nodiscard]] static std::string newTabButtonAccessibleLabel();

    /// @brief Genera una sugerencia de remediación clara y accesible en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(TabStripDensityError error,
                                                             std::string_view details = {});

    /// @brief Sanitiza el título de la pestaña eliminando credenciales, parámetros sensibles y hashes.
    [[nodiscard]] static std::string sanitizeTitle(std::string_view rawTitle, bool isIncognito = false);
};

} // namespace vigilio
