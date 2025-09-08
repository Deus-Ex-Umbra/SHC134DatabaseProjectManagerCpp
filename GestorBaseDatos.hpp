#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Modelos.hpp"
#include <libpq-fe.h>

class GestorBaseDatos {
public:
    GestorBaseDatos(const std::string& info_conexion);
    ~GestorBaseDatos();
    bool estaConectado();
    std::vector<Tabla> obtenerEsquemaTablas();
    void analizarDependenciasParaJwt(std::vector<Tabla>& tablas);

private:
    PGconn* conexion;
    std::string aPascalCase(const std::string& entrada);
    std::string aCamelCase(const std::string& entrada);
    std::string aKebabCase(const std::string& entradaPascalCase);
    std::string mapearTipoDbATs(const std::string& tipo_db);
    std::vector<std::string> obtenerNombresDeTablas();
    std::vector<Columna> obtenerColumnasParaTabla(const std::string& nombre_tabla);
    void obtenerDependenciasFk(std::vector<Tabla>& tablas);
};