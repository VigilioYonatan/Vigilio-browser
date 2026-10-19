#pragma once

#include "core/MainWindowLayoutTypes.h"
#include "core/TitleBarConfiguration.h"
#include "core/TitleBarTypes.h"

#include <string>
#include <string_view>

namespace vigilio {

/// @brief Validador estático desacoplado, algoritmo de distribución espacial,
///        clasificación de zonas (hit-testing) y utilidades de textos accesibles en español neutro.
class TitleBarValidator {
  public:
    /// @brief Valida que las dimensiones cumplan con los límites mínimos según la densidad activa.
    [[nodiscard]] static TitleBarError validateDimensions(int width, int height,
                                                          LayoutDensity density) noexcept;

    /// @brief Valida que el ancho de ventana cumpla con los límites mínimos según la densidad activa.
    [[nodiscard]] static TitleBarError validateDimensions(int width, LayoutDensity density) noexcept;

    /// @brief Realiza una auditoría estructural integral sobre un plan de barra de título.
    [[nodiscard]] static TitleBarError validatePlan(const TitleBarLayoutPlan& plan) noexcept;

    /// @brief Computa la distribución geométrica analítica de zonas y controles para la barra de título.
    [[nodiscard]] static TitleBarReport calculateTitleBarLayout(int windowWidth,
                                                                const TitleBarConfig& config);

    /// @brief Clasifica analíticamente un punto de coordenadas relativas (x, y) en su zona de impacto.
    [[nodiscard]] static TitleBarHitTestZone hitTest(int x, int y,
                                                     const TitleBarLayoutPlan& plan) noexcept;

    /// @brief Convierte un error de barra de título en un mensaje comprensible en español neutro.
    [[nodiscard]] static std::string titleBarErrorToString(TitleBarError error);

    /// @brief Convierte una modalidad operativa de barra de título en su denominación en español neutro.
    [[nodiscard]] static std::string titleBarModeToString(TitleBarMode mode);

    /// @brief Convierte una zona de impacto en su denominación descriptiva en español neutro.
    [[nodiscard]] static std::string hitTestZoneToString(TitleBarHitTestZone zone);

    /// @brief Convierte el tipo de botón en su etiqueta pública en español neutro.
    [[nodiscard]] static std::string buttonTypeToString(TitleBarButtonType type,
                                                        bool isMaximized = false);

    /// @brief Devuelve la etiqueta accesible para lectores de pantalla en español neutro.
    [[nodiscard]] static std::string buttonAccessibleLabel(TitleBarButtonType type,
                                                           bool isMaximized = false);

    /// @brief Devuelve la descripción flotante (tooltip) en español neutro.
    [[nodiscard]] static std::string buttonTooltip(TitleBarButtonType type, bool isMaximized = false);

    /// @brief Genera una sugerencia de remediación clara y accesible en español neutro.
    [[nodiscard]] static std::string generateRemediationHint(TitleBarError error,
                                                             std::string_view details = {});

    /// @brief Sanitiza el título de la ventana eliminando parámetros sensibles, tokens y hashes URL.
    [[nodiscard]] static std::string sanitizeTitle(std::string_view rawTitle, bool isIncognito = false);
};

} // namespace vigilio
