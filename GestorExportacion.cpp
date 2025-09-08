#include "GestorExportacion.hpp"
#include <iostream>
#include <cstdlib>

GestorExportacion::GestorExportacion(
    GestorAuditoria::MotorDB motor,
    const std::string& db, const std::string& user, const std::string& pass,
    const std::string& host, const std::string& port)
    : motor_db(motor), db_name(db), db_user(user), db_pass(pass), db_host(host), db_port(port)
{
}

bool GestorExportacion::exportarRespaldo(const std::string& ruta_archivo_salida) {
    std::string comando_dump;
    std::cout << "Iniciando exportacion completa de la base de datos..." << std::endl;

    switch (motor_db) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        comando_dump = "set PGPASSWORD=" + db_pass + "&& pg_dump -h " + db_host + " -p " + db_port + " -U " + db_user + " -F p -f \"" + ruta_archivo_salida + "\" " + db_name;
        break;
    case GestorAuditoria::MotorDB::MySQL:
        comando_dump = "mysqldump -h " + db_host + " -P " + db_port + " -u " + db_user + " -p" + db_pass + " --routines --triggers " + db_name + " > \"" + ruta_archivo_salida + "\"";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        std::cerr << "Advertencia: La exportacion automatica para SQL Server no esta soportada. Se requiere un proceso manual." << std::endl;
        return false;
    case GestorAuditoria::MotorDB::SQLite:
        comando_dump = "sqlite3 " + db_name + " .dump > \"" + ruta_archivo_salida + "\"";
        break;
    }

    std::cout << "Ejecutando comando: " << comando_dump << std::endl;
    int resultado = system(comando_dump.c_str());

    if (resultado != 0) {
        std::cerr << "Error: El comando de respaldo fallo. Verifique que la herramienta de linea de comandos este en el PATH." << std::endl;
        return false;
    }

    std::cout << "Exportacion completada. Respaldo guardado en: " << ruta_archivo_salida << std::endl;
    return true;
}