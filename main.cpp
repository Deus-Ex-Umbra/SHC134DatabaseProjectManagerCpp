#include <iostream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "Utils.hpp"

namespace po = boost::program_options;

std::string construirCadenaConexion(const po::variables_map& vm, GestorAuditoria::MotorDB motor) {
    std::string host = vm["host"].as<std::string>();
    std::string dbname = vm["dbname"].as<std::string>();
    std::string user = vm["user"].as<std::string>();
    std::string password = vm["password"].as<std::string>();
    std::string port;
    if (vm.count("port")) {
        port = vm["port"].as<std::string>();
    }
    else {
        if (motor == GestorAuditoria::MotorDB::PostgreSQL) port = "5432";
        if (motor == GestorAuditoria::MotorDB::MySQL) port = "3306";
        if (motor == GestorAuditoria::MotorDB::SQLServer) port = "1433";
    }

    if (motor == GestorAuditoria::MotorDB::PostgreSQL) {
        return "host=" + host +
            " port=" + port +
            " dbname=" + dbname +
            " user=" + user +
            " password=" + password;
    }
    else if (motor == GestorAuditoria::MotorDB::MySQL) {
        return "DRIVER={MySQL ODBC 8.0 Unicode Driver};"
            "SERVER=" + host + ";"
            "PORT=" + port + ";"
            "DATABASE=" + dbname + ";"
            "USER=" + user + ";"
            "PASSWORD=" + password + ";";
    }
    else if (motor == GestorAuditoria::MotorDB::SQLServer) {
        std::string driver = vm.count("driver") ? vm["driver"].as<std::string>() : "ODBC Driver 17 for SQL Server";
        return "DRIVER={" + driver + "};"
            "SERVER=" + host + ";"
            "PORT=" + port + ";"
            "DATABASE=" + dbname + ";"
            "UID=" + user + ";"
            "PWD=" + password + ";"
            "Encrypt=yes;"
            "TrustServerCertificate=yes;";
    }

    else if (motor == GestorAuditoria::MotorDB::SQLite) {
        std::string db_path = dbname;
        if (db_path.find(".db") == std::string::npos && db_path.find(".sqlite") == std::string::npos) {
            db_path += ".sqlite";
        }
        return "DRIVER={SQLite3 ODBC Driver};"
            "DATABASE=" + db_path + ";";
    }

    throw std::runtime_error("Motor de base de datos no soportado.");
}

GestorAuditoria::MotorDB obtenerMotorDB(const std::string& motor_str) {
    std::string motor_lower = boost::to_lower_copy(motor_str);
    if (motor_lower == "postgres" || motor_lower == "postgresql")
        return GestorAuditoria::MotorDB::PostgreSQL;
    if (motor_lower == "mysql")
        return GestorAuditoria::MotorDB::MySQL;
    if (motor_lower == "sqlserver" || motor_lower == "mssql")
        return GestorAuditoria::MotorDB::SQLServer;
    if (motor_lower == "sqlite" || motor_lower == "sqlite3")
        return GestorAuditoria::MotorDB::SQLite;
    throw std::runtime_error("Motor no reconocido: " + motor_str);
}

void imprimirEncabezado(const po::variables_map& vm) {
    std::cout << "\nSHC134 Database Project Manager v1.0" << std::endl;
    std::cout << "Accion: " << vm["accion"].as<std::string>() << std::endl;
    std::cout << "Motor: " << vm["motor"].as<std::string>() << std::endl;
    std::cout << "Base de datos: " << vm["dbname"].as<std::string>() << std::endl;
    std::cout << "=========================================" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("Opciones disponibles");
        desc.add_options()
            ("help,h", "Muestra esta ayuda")
            ("accion", po::value<std::string>()->required(),
                "Accion a realizar: scaffolding, auditoria, encriptado, sql")
            ("motor", po::value<std::string>()->default_value("postgres"),
                "Motor de base de datos: postgres, mysql, sqlserver, sqlite")
            ("host", po::value<std::string>()->default_value("localhost"),
                "Host del servidor de base de datos")
            ("port", po::value<std::string>(),
                "Puerto del servidor")
            ("user", po::value<std::string>()->default_value("postgres"),
                "Usuario de la base de datos")
            ("password", po::value<std::string>()->default_value(""),
                "Contrasena del usuario")
            ("dbname", po::value<std::string>()->required(),
                "Nombre de la base de datos")
            ("tabla", po::value<std::string>(),
                "Nombre de tabla especifica (para auditoria)")
            ("key", po::value<std::string>(),
                "Clave de encriptacion en hexadecimal (64 caracteres)")
            ("encrypt-audit-tables",
                "Cifrar las tablas de auditoria existentes")
            ("query", po::value<std::string>(),
                "Consulta SQL a ejecutar")
            ("out", po::value<std::string>(),
                "Directorio de salida para scaffolding")
            ("jwt-secret", po::value<std::string>(),
                "Secreto JWT para autenticacion")
            ("driver", po::value<std::string>(),
                "Driver ODBC especifico (para SQL Server)");

        po::positional_options_description pos;
        pos.add("accion", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        std::string accion = boost::to_lower_copy(vm["accion"].as<std::string>());

        if (accion != "scaffolding" && accion != "auditoria" &&
            accion != "encriptado" && accion != "sql") {
            throw std::runtime_error("Accion no valida: " + accion);
        }

        imprimirEncabezado(vm);

        GestorAuditoria::MotorDB motor = obtenerMotorDB(vm["motor"].as<std::string>());
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
        else if (accion == "sql") {
            std::cout << "Ejecutando consulta SQL..." << std::endl;
            manejarConsultaSql(vm, motor, info_conexion);
        }

        std::cout << "\nProceso completado exitosamente." << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}