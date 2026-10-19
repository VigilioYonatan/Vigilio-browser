#pragma once

#include "core/CiWindowsJobTypes.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace vigilio {

/// @brief Capa operativa propietaria desacoplada para la orquestación y ejecución de trabajos de CI en
/// Windows.
///
/// Proporciona planificación, validación previa, ejecución determinista en modos seco (DryRun), simulado
/// y en vivo, soporte de cancelación cooperativa multihilo bajo la política Zero-Zombie, corte rápido
/// (fail-fast), tolerancia selectiva a fallos (continueOnError), saneamiento seguro de directorios y
/// síntesis reproducible de CLI, sin dependencias hacia componentes de interfaz gráfica (QApplication /
/// QWidget) ni Chromium WebEngine.
class CiWindowsJobService {
  public:
    /// @brief Construye el servicio inicializado con el plan canónico de depuración en Windows.
    CiWindowsJobService();

    /// @brief Construye el servicio inyectando un plan de trabajo explícito.
    explicit CiWindowsJobService(CiWindowsJobPlan plan);

    ~CiWindowsJobService() = default;

    CiWindowsJobService(const CiWindowsJobService&) = delete;
    CiWindowsJobService& operator=(const CiWindowsJobService&) = delete;
    CiWindowsJobService(CiWindowsJobService&&) = delete;
    CiWindowsJobService& operator=(CiWindowsJobService&&) = delete;

    /// @brief Asigna un nuevo plan de trabajo con protección de concurrencia mediante mutex.
    void setPlan(CiWindowsJobPlan plan);

    /// @brief Obtiene una copia inmutable del plan de trabajo actualmente configurado.
    [[nodiscard]] CiWindowsJobPlan plan() const;

    /// @brief Asigna las opciones operativas de ejecución del servicio.
    void setOptions(CiWindowsServiceOptions options);

    /// @brief Obtiene las opciones operativas configuradas.
    [[nodiscard]] CiWindowsServiceOptions options() const;

    /// @brief Ejecuta el plan configurado respetando el orden topológico, fail-fast y límites de
    /// recursos.
    /// @param cancel Puntero opcional al token de cancelación cooperativa (Zero-Zombie).
    /// @return Reporte consolidado inmutable de la ejecución del trabajo.
    [[nodiscard]] CiWindowsJobReport executePlan(CancellationToken* cancel = nullptr);

    /// @brief Ejecuta una etapa individual de manera aislada para diagnóstico o prueba.
    /// @param step Etapa concreta a evaluar.
    /// @param cancel Puntero opcional al token de cancelación cooperativa.
    /// @return Resultado individual inmutable de la ejecución de la etapa.
    [[nodiscard]] CiWindowsStepResult executeStep(CiWindowsPipelineStep step,
                                                  CancellationToken* cancel = nullptr);

    /// @brief Limpia de manera segura y recursiva el directorio de construcción configurado.
    /// @param errorMessage Mensaje opcional para capturar detalles en caso de fallo.
    /// @return true si la limpieza fue exitosa; false en caso de error o ruta insegura.
    [[nodiscard]] bool cleanBuildDirectory(std::string* errorMessage = nullptr);

    /// @brief Sintetiza de manera determinista la línea de comandos CLI para una etapa dada.
    [[nodiscard]] std::string buildCommandLine(const CiWindowsStepDefinition& step) const;

    /// @brief Convierte un modo de ejecución a texto descriptivo en español neutro (UX copy).
    [[nodiscard]] static std::string executionModeToString(CiWindowsExecutionMode mode);

    /// @brief Formatea un resumen accesible y legible de un resultado de etapa en español neutro.
    [[nodiscard]] static std::string formatStepSummary(const CiWindowsStepResult& result);

  private:
    CiWindowsJobPlan plan_;
    CiWindowsServiceOptions options_;
    mutable std::mutex mutex_;

    [[nodiscard]] CiWindowsStepResult executeStepInternal(const CiWindowsStepDefinition& stepDef,
                                                          const CiWindowsJobPlan& currentPlan,
                                                          const CiWindowsServiceOptions& currentOpts,
                                                          CancellationToken* cancel);
};

// Funciones libres para ergonomía en namespace vigilio
[[nodiscard]] inline std::string executionModeToString(CiWindowsExecutionMode mode) {
    return CiWindowsJobService::executionModeToString(mode);
}

[[nodiscard]] inline std::string formatStepSummary(const CiWindowsStepResult& result) {
    return CiWindowsJobService::formatStepSummary(result);
}

} // namespace vigilio
