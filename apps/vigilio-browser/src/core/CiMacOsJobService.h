#pragma once

#include "core/CiMacOsJobTypes.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa propietaria desacoplada para la orquestación y ejecución de trabajos de CI en
/// macOS.
///
/// Proporciona planificación, validación previa, ejecución determinista en modos seco (DryRun), simulado
/// y en vivo, soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie, corte rápido
/// (fail-fast), tolerancia selectiva a fallos (continueOnError), saneamiento seguro de directorios y
/// síntesis reproducible de CLI adaptada a las herramientas nativas de macOS (CMake, Ninja, Clang,
/// CTest, bundle nativo .app), sin dependencias hacia componentes de interfaz gráfica (QApplication /
/// QWidget) ni Chromium WebEngine.
class CiMacOsJobService {
  public:
    /// @brief Construye el servicio inicializado con el plan canónico de depuración en macOS.
    CiMacOsJobService();

    /// @brief Construye el servicio inyectando un plan de trabajo explícito.
    explicit CiMacOsJobService(CiMacOsJobPlan plan);

    ~CiMacOsJobService() = default;

    CiMacOsJobService(const CiMacOsJobService&) = delete;
    CiMacOsJobService& operator=(const CiMacOsJobService&) = delete;
    CiMacOsJobService(CiMacOsJobService&&) = delete;
    CiMacOsJobService& operator=(CiMacOsJobService&&) = delete;

    /// @brief Asigna un nuevo plan de trabajo con protección de concurrencia mediante mutex.
    void setPlan(CiMacOsJobPlan plan);

    /// @brief Obtiene una copia inmutable del plan de trabajo actualmente configurado.
    [[nodiscard]] CiMacOsJobPlan plan() const;

    /// @brief Asigna las opciones operativas de ejecución del servicio.
    void setOptions(CiMacOsServiceOptions options);

    /// @brief Obtiene las opciones operativas configuradas.
    [[nodiscard]] CiMacOsServiceOptions options() const;

    /// @brief Ejecuta el plan configurado respetando el orden topológico, fail-fast y límites de
    /// recursos.
    /// @param cancel Puntero opcional al token de cancelación cooperativa (Zero-Zombie).
    /// @return Reporte consolidado inmutable de la ejecución del trabajo.
    [[nodiscard]] CiMacOsJobReport executePlan(CancellationToken* cancel = nullptr);

    /// @brief Ejecuta una etapa individual de manera aislada para diagnóstico o prueba.
    /// @param step Etapa concreta a evaluar.
    /// @param cancel Puntero opcional al token de cancelación cooperativa.
    /// @return Resultado individual inmutable de la ejecución de la etapa.
    [[nodiscard]] CiMacOsStepResult executeStep(CiMacOsPipelineStep step,
                                                CancellationToken* cancel = nullptr);

    /// @brief Limpia de manera segura y recursiva el directorio de construcción configurado con defensas
    /// específicas de macOS.
    /// @param errorMessage Mensaje opcional para capturar detalles en caso de fallo.
    /// @return true si la limpieza fue exitosa; false en caso de error o ruta insegura.
    [[nodiscard]] bool cleanBuildDirectory(std::string* errorMessage = nullptr);

    /// @brief Sintetiza de manera determinista la línea de comandos CLI para una etapa dada en macOS.
    [[nodiscard]] std::string buildCommandLine(const CiMacOsStepDefinition& step) const;

    /// @brief Convierte un modo de ejecución a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string executionModeToString(CiMacOsExecutionMode mode);

    /// @brief Formatea un resumen accesible y legible de un resultado de etapa en español neutro.
    [[nodiscard]] static std::string formatStepSummary(const CiMacOsStepResult& result);

  private:
    CiMacOsJobPlan plan_;
    CiMacOsServiceOptions options_;
    mutable std::mutex mutex_;

    [[nodiscard]] CiMacOsStepResult executeStepInternal(const CiMacOsStepDefinition& stepDef,
                                                        const CiMacOsJobPlan& currentPlan,
                                                        const CiMacOsServiceOptions& currentOpts,
                                                        CancellationToken* cancel);
};

// Funciones libres para ergonomía en namespace vigilio
[[nodiscard]] inline std::string executionModeToString(CiMacOsExecutionMode mode) {
    return CiMacOsJobService::executionModeToString(mode);
}

[[nodiscard]] inline std::string formatStepSummary(const CiMacOsStepResult& result) {
    return CiMacOsJobService::formatStepSummary(result);
}

} // namespace vigilio
