#pragma once
#include <string>
#include "GestorAuditoria.hpp"

class GestorExportacion {
public:
    GestorExportacion(
        GestorAuditoria::MotorDB motor,
        const std::string& db, const std::string& user, const std::string& pass,
        const std::string& host, const std::string& port
    );

    bool exportarRespaldo(const std::string& ruta_archivo_salida);

private:
    GestorAuditoria::MotorDB motor_db;
    std::string db_user, db_pass, db_name, db_host, db_port;
};