#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include "Utils.hpp"
#include "GestorBaseDatos.hpp"
#include "GeneradorCodigo.hpp"
#include "GestorAuditoria.hpp"
#include "GestorCifrado.hpp"
#include "GestorSeguridad.hpp"

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;

    po::options_description desc("Opciones permitidas");
    desc.add_options()
        ("help,h", "Produce este mensaje de ayuda")

        ("accion", po::value<std::string>(), "Accion a realizar: scaffold, audit, encrypt, execute")

        ("motor", po::value<std::string>()->default_value("postgres"), "Motor DB (postgres, mysql, sqlite)")
        ("host", po::value<std::string>()->default_value("localhost"), "Host de la base de datos")
        ("port", po::value<std::string>()->default_value("5450"), "Puerto de la base de datos")
        ("dbname", po::value<std::string>()->default_value("nest_db"), "Nombre de la base de datos / Archivo para SQLite")
        ("user", po::value<std::string>()->default_value("root"), "Usuario de la base de datos")
        ("password", po::value<std::string>()->default_value("root"), "Contraseña de la base de datos")

        ("jwt-secret", po::value<std::string>(), "[Scaffold] Clave secreta para firmar los JWT")
        ("out,o", po::value<std::string>()->default_value("api-generada-nest"), "[Scaffold] Directorio de salida")

        ("tabla", po::value<std::string>(), "[Audit/Encrypt] Tabla especifica a procesar (opcional)")

        ("encryption-key", po::value<std::string>(), "[Encrypt/Execute] Clave de 32 caracteres para el cifrado AES-256")
        ("columnas", po::value<std::string>(), "[Encrypt] Columnas a cifrar (separadas por coma)")
        ("query", po::value<std::string>(), "[Execute] Consulta SQL a ejecutar")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return 1;
    }

    if (vm.count("help") || !vm.count("accion")) {
        std::cout << desc << std::endl;
        return 0;
    }

    const std::string accion = vm["accion"].as<std::string>();
    const std::string motor_str = vm["motor"].as<std::string>();
    GestorAuditoria::MotorDB motor;

    if (motor_str == "postgres") motor = GestorAuditoria::MotorDB::PostgreSQL;
    else if (motor_str == "mysql") motor = GestorAuditoria::MotorDB::MySQL;
    else if (motor_str == "sqlite") motor = GestorAuditoria::MotorDB::SQLite;
    else {
        std::cerr << "Motor de base de datos no soportado: " << motor_str << std::endl;
        return 1;
    }

    std::stringstream ss_conexion;
    if (motor == GestorAuditoria::MotorDB::PostgreSQL) {
        ss_conexion << "postgresql://" << vm["user"].as<std::string>() << ":" << vm["password"].as<std::string>() << "@" << vm["host"].as<std::string>() << ":" << vm["port"].as<std::string>() << "/" << vm["dbname"].as<std::string>();
    }
    else if (motor == GestorAuditoria::MotorDB::MySQL) {
        ss_conexion << "mysqlx://" << vm["user"].as<std::string>() << ":" << vm["password"].as<std::string>() << "@" << vm["host"].as<std::string>() << ":" << vm["port"].as<std::string>() << "/" << vm["dbname"].as<std::string>();
    }
    else if (motor == GestorAuditoria::MotorDB::SQLite) {
        ss_conexion << "Driver=SQLite3;Database=" << vm["dbname"].as<std::string>();
    }
    const std::string INFO_CONEXION = ss_conexion.str();

    try {
        if (accion == "scaffold") {
            if (!vm.count("jwt-secret")) throw std::runtime_error("La clave secreta para JWT (--jwt-secret) es obligatoria.");

            GestorBaseDatos gestor_db(INFO_CONEXION);
            if (!gestor_db.estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

            std::cout << "Conectado. Obteniendo esquema de tablas..." << std::endl;
            std::vector<Tabla> esquema_tablas = gestor_db.obtenerEsquemaTablas();
            if (esquema_tablas.empty()) throw std::runtime_error("No se encontraron tablas.");

            const std::string dir_salida = vm["out"].as<std::string>();
            GeneradorCodigo generador(dir_salida);
            generador.generarProyectoCompleto(esquema_tablas);

            std::ofstream archivo_env(dir_salida + "/.env");
            archivo_env << "JWT_SECRET=" + vm["jwt-secret"].as<std::string>() + "\n";
            archivo_env.close();

            std::cout << "\nInstalando dependencias (npm install)..." << std::endl;
            ejecutarComando("cd " + dir_salida + " && npm install");
            imprimirRutasApi(esquema_tablas, dir_salida);
            ejecutarComando("cd " + dir_salida + " && npm run start:dev", false);

        }
        else if (accion == "audit") {
            auto gestor_auditoria = std::make_shared<GestorAuditoria>(motor, INFO_CONEXION, vm["dbname"].as<std::string>());
            if (!gestor_auditoria->estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

            if (vm.count("tabla")) {
                std::cout << "Generando auditoria para la tabla: " << vm["tabla"].as<std::string>() << std::endl;
                gestor_auditoria->generarAuditoriaParaTabla(vm["tabla"].as<std::string>());
            }
            else {
                std::vector<std::string> tablas = gestor_auditoria->obtenerNombresDeTablas();
                for (const auto& tabla : tablas) {
                    std::cout << "Generando auditoria para la tabla: " << tabla << std::endl;
                    gestor_auditoria->generarAuditoriaParaTabla(tabla);
                }
            }
            std::cout << "Proceso de auditoria completado." << std::endl;

        }
        else if (accion == "encrypt") {
            if (!vm.count("encryption-key")) throw std::runtime_error("La clave de cifrado (--encryption-key) es obligatoria.");
            std::cout << "Esta funcionalidad es solo una DEMOSTRACION del cifrado a nivel de aplicacion." << std::endl;
            // La implementación real estaría en la API generada. Aquí solo mostramos cómo funcionaría.
            GestorCifrado gestor_cifrado(nullptr, vm["encryption-key"].as<std::string>());
            // gestor_cifrado.cifrarTabla(...);

        }
        else if (accion == "execute") {
            if (!vm.count("encryption-key") || !vm.count("query")) throw std::runtime_error("Se requieren --encryption-key y --query.");

            auto gestor_auditoria = std::make_shared<GestorAuditoria>(motor, INFO_CONEXION, vm["dbname"].as<std::string>());
            if (!gestor_auditoria->estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

            GestorCifrado gestor_cifrado(gestor_auditoria, vm["encryption-key"].as<std::string>());

            auto resultados = gestor_cifrado.ejecutarConsultaYDescifrar(vm["query"].as<std::string>(), {});

            for (const auto& fila : resultados) {
                for (const auto& celda : fila) {
                    std::cout << celda << "\t";
                }
                std::cout << std::endl;
            }
        }
        else {
            std::cerr << "Accion no reconocida: " << accion << std::endl;
            std::cout << desc << std::endl;
            return 1;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error critico: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}