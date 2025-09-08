#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Modelos.hpp"

class GeneradorCodigo {
public:
    GeneradorCodigo(const std::string& dir_salida);
    void generarProyectoCompleto(const std::vector<Tabla>& tablas);

private:
    std::string dir_salida;
    void generarArchivosBase(const nlohmann::json& datos_modulos);
    void generarModuloAutenticacion(const Tabla& tabla_usuario);
    void generarModuloCrud(const Tabla& tabla);
    void escribirArchivo(const std::string& ruta, const std::string& contenido);
    std::string renderizarPlantilla(const std::string& ruta_plantilla, const nlohmann::json& datos);
};