#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include "Utils.hpp"
#include "GestorAuditoria.hpp"

namespace po = boost::program_options;

std::string construirCadenaConexion(const po::variables_map& vm, GestorAuditoria::MotorDB motor) {
    std::string cadena_conexion;

    switch (motor) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        cadena_conexion = "host=" + vm["host"].as<std::string>() +
            " port=" + vm["port"].as<std::string>() +
            " dbname=" + vm["dbname"].as<std::string>() +
            " user=" + vm["user"].as<std::string>() +
            " password='" + vm["password"].as<std::string>() + "'";
        break;

    case GestorAuditoria::MotorDB::MySQL:
        cadena_conexion = "mysql://" + vm["user"].as<std::string>() + ":" +
            vm["password"].as<std::string>() + "@" +
            vm["host"].as<std::string>() + ":" +
            vm["port"].as<std::string>() + "/" +
            vm["dbname"].as<std::string>();
        break;

    case GestorAuditoria::MotorDB::SQLServer:
        cadena_conexion = "DRIVER={ODBC Driver 17 for SQL Server};SERVER=" +
            vm["host"].as<std::string>() + "," + vm["port"].as<std::string>() +
            ";DATABASE=" + vm["dbname"].as<std::string>() +
            ";UID=" + vm["user"].as<std::string>() +
            ";PWD=" + vm["password"].as<std::string>();
        break;

    case GestorAuditoria::MotorDB::SQLite:
        cadena_conexion = "DRIVER={SQLite3 ODBC Driver};Database=" + vm["dbname"].as<std::string>();
        break;
    }

    return cadena_conexion;
}

GestorAuditoria::MotorDB obtenerMotorDb(const std::string& motor_str) {
    if (motor_str == "postgres" || motor_str == "postgresql") {
        return GestorAuditoria::MotorDB::PostgreSQL;
    }
    else if (motor_str == "mysql") {
        return GestorAuditoria::MotorDB::MySQL;
    }
    else if (motor_str == "sqlserver" || motor_str == "mssql") {
        return GestorAuditoria::MotorDB::SQLServer;
    }
    else if (motor_str == "sqlite") {
        return GestorAuditoria::MotorDB::SQLite;
    }
    else {
        throw std::runtime_error("Motor de base de datos no soportado: " + motor_str);
    }
}

void mostrarAyuda(const po::options_description& desc) {
    std::cout << "SHC134 Database Project Manager" << std::endl;
    std::cout << "Herramienta para scaffolding, auditoria y cifrado de bases de datos" << std::endl;
    std::cout << std::endl;
    std::cout << "Uso: programa.exe <accion> [opciones]" << std::endl;
    std::cout << std::endl;
    std::cout << "Acciones disponibles:" << std::endl;
    std::cout << "  scaffolding    Genera proyecto completo de API con Nest.js" << std::endl;
    std::cout << "  auditoria      Crea tablas de auditoria y triggers" << std::endl;
    std::cout << "  encriptado     Gestiona cifrado de tablas de auditoria" << std::endl;
    std::cout << std::endl;
    std::cout << desc << std::endl;
    std::cout << std::endl;
    std::cout << "Ejemplos:" << std::endl;
    std::cout << "  programa.exe scaffolding --motor postgres --dbname mi_db --jwt-secret \"clave123\"" << std::endl;
    std::cout << "  programa.exe auditoria --motor mysql --dbname mi_db --user admin --password pass" << std::endl;
    std::cout << "  programa.exe encriptado --motor postgres --dbname mi_db --key \"64chars\" --encrypt-audit-tables" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("Opciones disponibles");
        desc.add_options()
            ("help,h", "Muestra esta ayuda")
            ("motor", po::value<std::string>()->default_value("postgres"), "Motor de BD (postgres, mysql, sqlserver, sqlite)")
            ("host", po::value<std::string>()->default_value("localhost"), "Host del servidor")
            ("port", po::value<std::string>()->default_value("5432"), "Puerto del servidor")
            ("dbname", po::value<std::string>()->default_value("nest_db"), "Nombre de la base de datos")
            ("user", po::value<std::string>()->default_value("root"), "Usuario")
            // CORRECCIÓN 1: Manejo explícito de contraseñas vacías.
            ("password", po::value<std::string>()->default_value("")->implicit_value(""), "Contraseña")
            ("out,o", po::value<std::string>(), "Directorio de salida (solo scaffolding)")
            ("jwt-secret", po::value<std::string>(), "Clave secreta JWT (requerido para scaffolding)")
            ("tabla", po::value<std::string>(), "Tabla específica a auditar")
            ("key,k", po::value<std::string>(), "Clave de cifrado hexadecimal (64 caracteres)")
            ("encrypt-audit-tables", "Cifra todas las tablas de auditoria")
            ("query,q", po::value<std::string>(), "Consulta SQL para ejecutar con descifrado");

        if (argc < 2) {
            mostrarAyuda(desc);
            return 1;
        }

        std::string accion = argv[1];

        po::variables_map vm;
        po::store(po::parse_command_line(argc - 1, argv + 1, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            mostrarAyuda(desc);
            return 0;
        }

        std::cout << "SHC134 Database Project Manager v1.0" << std::endl;
        std::cout << "Accion: " << accion << std::endl;
        std::cout << "Motor: " << vm["motor"].as<std::string>() << std::endl;
        std::cout << "Base de datos: " << vm["dbname"].as<std::string>() << std::endl;
        std::cout << "=========================================" << std::endl;

        GestorAuditoria::MotorDB motor = obtenerMotorDb(vm["motor"].as<std::string>());
        std::string info_conexion = construirCadenaConexion(vm, motor);

        if (accion == "scaffolding") {
            std::cout << "Iniciando generacion de scaffolding..." << std::endl;
            manejarScaffolding(vm, motor, info_conexion);
        }
        else if (accion == "auditoria") {
            std::cout << "Iniciando proceso de auditoria..." << std::endl;
            manejarAuditoria(vm, motor, info_conexion);
        }
        else if (accion == "encriptado") {
            std::cout << "Iniciando proceso de encriptado..." << std::endl;
            manejarEncriptado(vm, motor, info_conexion);
        }
        else {
            std::cerr << "Accion no reconocida: " << accion << std::endl;
            std::cerr << "Acciones validas: scaffolding, auditoria, encriptado" << std::endl;
            return 1;
        }

        std::cout << std::endl << "Proceso completado exitosamente." << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}