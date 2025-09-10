#pragma once
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <nlohmann/json.hpp>
#include <inja/inja.hpp>
#include <libpq-fe.h>
#include <nanodbc/nanodbc.h>
#include "GestorCifrado.hpp"

struct ResultadoConsulta {
    std::vector<std::string> columnas;
    std::vector<std::vector<std::string>> filas;
};

class GestorCifrado;

class GestorAuditoria {
public:
    enum class MotorDB {
        PostgreSQL,
        SQLServer,
        MySQL,
        SQLite
    };

    GestorAuditoria(MotorDB motor, const std::string& connection_string, const std::string& db = "");
    ~GestorAuditoria();

    bool estaConectado() const;
    std::vector<std::string> obtenerNombresDeTablas(bool incluir_auditoria);
    void generarAuditoriaParaTabla(const std::string& nombre_tabla);
    ResultadoConsulta ejecutarConsultaConResultado(const std::string& consulta);
    void ejecutarComando(const std::string& consulta);
    MotorDB getMotor() const;
    void setGestorCifrado(std::shared_ptr<GestorCifrado> gestor);

private:
    MotorDB motor_actual;
    std::string db_name;
    inja::Environment env_plantillas;
    std::shared_ptr<GestorCifrado> gestor_cifrado;

    PGconn* conn_pg = nullptr;
    std::unique_ptr<nanodbc::connection> conn_odbc;

    void conectar(const std::string& connection_string, const std::string& db);
    void desconectar();

    void crearFuncionesAuditoria();
    void crearFuncionesAuditoriaMySQL();
    void crearFuncionesAuditoriaSQLServer();
    void generarAuditoriaPostgreSQL(const std::string& nombre_tabla);
    void generarAuditoriaSQLServer(const std::string& nombre_tabla);
    void generarAuditoriaMySQL(const std::string& nombre_tabla);
    void generarAuditoriaSQLite(const std::string& nombre_tabla);
};