#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <boost/program_options.hpp>
#include "Utils.hpp"

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("Uso: SHC134DatabaseProjectManagerCpp <accion> [opciones]\nAcciones: scaffolding, auditoria, encriptado, exportar");
    desc.add_options()
        ("help,h", "Ayuda")
        ("accion", po::value<std::string>(), "Accion a realizar")
        ("motor", po::value<std::string>()->default_value("postgres"), "Motor DB (postgres, mysql, sqlite)")
        ("host", po::value<std::string>()->default_value("localhost"), "Host DB")
        ("port", po::value<std::string>()->default_value("5450"), "Puerto DB")
        ("dbname", po::value<std::string>()->default_value("nest_db"), "Nombre DB")
        ("user", po::value<std::string>()->default_value("root"), "Usuario DB")
        ("password", po::value<std::string>()->default_value("root"), "Contraseña DB")
        ("out,o", po::value<std::string>(), "Archivo/Directorio de salida")
        ("jwt-secret", po::value<std::string>(), "[Scaffolding] Clave para JWT")
        ("tabla", po::value<std::string>(), "[Auditoria] Tabla especifica")
        ("key,k", po::value<std::string>(), "[Encriptado] Clave HEX de 64 caracteres")
        ("query,q", po::value<std::string>(), "[Encriptado] Consulta SQL a ejecutar/descifrar")
        ("encrypt-audit-tables", po::bool_switch(), "[Encriptado] Cifra todas las tablas de auditoria existentes");

    po::positional_options_description p;
    p.add("accion", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);
    }
    catch (const po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << desc << std::endl;
        return 1;
    }

    if (vm.count("help") || !vm.count("accion")) {
        std::cout << desc << std::endl;
        return 0;
    }

    try {
        const std::string accion = vm["accion"].as<std::string>();
        const std::string motor_str = vm["motor"].as<std::string>();
        GestorAuditoria::MotorDB motor;
        if (motor_str == "postgres") motor = GestorAuditoria::MotorDB::PostgreSQL;
        else if (motor_str == "mysql") motor = GestorAuditoria::MotorDB::MySQL;
        else if (motor_str == "sqlite") motor = GestorAuditoria::MotorDB::SQLite;
        else throw std::runtime_error("Motor no soportado: " + motor_str);

        std::stringstream ss_conexion;
        if (motor == GestorAuditoria::MotorDB::PostgreSQL) ss_conexion << "postgresql://" << vm["user"].as<std::string>() << ":" << vm["password"].as<std::string>() << "@" << vm["host"].as<std::string>() << ":" << vm["port"].as<std::string>() << "/" << vm["dbname"].as<std::string>();
        else if (motor == GestorAuditoria::MotorDB::MySQL) ss_conexion << "Driver={MySQL ODBC 8.0 Unicode Driver};Server=" << vm["host"].as<std::string>() << ";Database=" << vm["dbname"].as<std::string>() << ";Uid=" << vm["user"].as<std::string>() << ";Pwd=" << vm["password"].as<std::string>() << ";";
        else if (motor == GestorAuditoria::MotorDB::SQLite) ss_conexion << "Driver=SQLite3;Database=" << vm["dbname"].as<std::string>();

        if (accion == "scaffolding") manejarScaffolding(vm, motor, ss_conexion.str());
        else if (accion == "auditoria") manejarAuditoria(vm, motor, ss_conexion.str());
        else if (accion == "encriptado") manejarEncriptado(vm, motor, ss_conexion.str());
        else if (accion == "exportar") manejarExportar(vm, motor);
        else throw std::runtime_error("Accion no reconocida: " + accion);

    }
    catch (const std::exception& e) {
        std::cerr << "Error critico: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}