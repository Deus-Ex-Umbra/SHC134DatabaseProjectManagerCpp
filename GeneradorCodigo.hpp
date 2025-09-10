#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Modelos.hpp"

class GeneradorCodigo {
public:
    GeneradorCodigo(const std::string& dir_salida);
    void generarProyectoCompleto(const std::vector<Tabla>& tablas, const std::string& motor_db, const std::string& host, const std::string& puerto, const std::string& usuario, const std::string& contrasena, const std::string& base_datos, const std::string& jwt_secret);

private:
    std::string dir_salida;
    void generarArchivosBase(const nlohmann::json& datos_modulos);
    void generarModuloAutenticacion(const Tabla& tabla_usuario);
    void generarModuloCrud(const Tabla& tabla, const std::vector<Tabla>& todas_las_tablas);
    void generarArchivoEnv(const std::string& motor_db, const std::string& host, const std::string& puerto, const std::string& usuario, const std::string& contrasena, const std::string& base_datos, const std::string& jwt_secret);
    void escribirArchivo(const std::string& ruta, const std::string& contenido);
    std::string renderizarPlantilla(const std::string& ruta_plantilla, const nlohmann::json& datos);
};