#pragma once

#include <string>
#include <vector>
#include "GestorAuditoria.hpp"

#include <openssl/evp.h>
#include <openssl/aes.h>

class GestorSeguridad {
public:
    GestorSeguridad(GestorAuditoria* gestor, GestorAuditoria::MotorDB motor,
        const std::string& db, const std::string& user, const std::string& pass,
        const std::string& host, const std::string& port);

    bool exportarYEncriptarRespaldo(const std::string& ruta_archivo_salida, const std::string& clave, const std::string& iv);

    bool desencriptarRespaldo(const std::string& ruta_archivo_encriptado, const std::string& ruta_archivo_desencriptado, const std::string& clave, const std::string& iv);

private:
    GestorAuditoria* gestor_db;
    GestorAuditoria::MotorDB motor_db;
    std::string db_user, db_pass, db_name, db_host, db_port;

    bool encriptarArchivo(const std::string& ruta_entrada, const std::string& ruta_salida, const unsigned char* clave, const unsigned char* iv);
    bool desencriptarArchivo(const std::string& ruta_entrada, const std::string& ruta_salida, const unsigned char* clave, const unsigned char* iv);
};