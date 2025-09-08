#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Modelos.hpp"
#include "GestorAuditoria.hpp"
#include <libpq-fe.h>
#include <nanodbc/nanodbc.h>

class GestorBaseDatos {
public:
    GestorBaseDatos(GestorAuditoria::MotorDB motor, const std::string& info_conexion, const std::string& dbname);
    ~GestorBaseDatos();
    bool estaConectado();
    std::vector<Tabla> obtenerEsquemaTablas();

private:
    GestorAuditoria::MotorDB motor_actual;
    std::string db_name;

    PGconn* conn_pg = nullptr;
    std::unique_ptr<nanodbc::connection> conn_odbc;

    std::string aPascalCase(const std::string& entrada);
    std::string aCamelCase(const std::string& entrada);
    std::string aKebabCase(const std::string& entradaPascalCase);
    std::string mapearTipoDbATs(const std::string& tipo_db);
    std::vector<std::string> obtenerNombresDeTablas();
    std::vector<Columna> obtenerColumnasParaTabla(const std::string& nombre_tabla);
    void obtenerDependenciasFk(std::vector<Tabla>& tablas);
    void analizarDependenciasParaJwt(std::vector<Tabla>& tablas);
};