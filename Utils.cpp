#include "Utils.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include "GestorBaseDatos.hpp"
#include "GeneradorCodigo.hpp"
#include "GestorCifrado.hpp"

void ejecutarComando(const std::string& comando, bool esperar) {
    std::cout << "Ejecutando: " << comando << std::endl;
    int resultado = system(comando.c_str());
    if (resultado != 0) {
        std::cerr << "Advertencia: Comando finalizo con codigo de error: " << resultado << std::endl;
    }
}

void imprimirRutasApi(const std::vector<Tabla>& tablas, const std::string& dir_salida) {
    std::cout << "\n--- Rutas de API Generadas ---" << std::endl;
    std::cout << "URL Base: http://localhost:3000\n" << std::endl;
    for (const auto& tabla : tablas) {
        if (tabla.es_tabla_usuario) {
            std::cout << "Autenticacion: POST /autenticacion/login" << std::endl;
        }
        std::string ruta = "/" + tabla.nombre_archivo;
        std::cout << "Recurso: " << tabla.nombre_clase << (tabla.es_protegida ? " (Protegida)" : " (Publica)") << std::endl;
        std::cout << "  POST " << ruta << ", GET " << ruta << ", GET " << ruta << "/:id, PATCH " << ruta << "/:id, DELETE " << ruta << "/:id" << std::endl;
    }
}

void manejarScaffolding(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion) {
    if (!vm.count("jwt-secret")) throw std::runtime_error("--jwt-secret es obligatorio.");

    GestorBaseDatos gestor_db(motor, info_conexion, vm["dbname"].as<std::string>());
    if (!gestor_db.estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

    std::vector<Tabla> esquema = gestor_db.obtenerEsquemaTablas();
    if (esquema.empty()) throw std::runtime_error("No se encontraron tablas.");

    const std::string dir_salida = vm.count("out") ? vm["out"].as<std::string>() : "api-generada-nest";
    GeneradorCodigo generador(dir_salida);
    generador.generarProyectoCompleto(esquema,
        boost::to_lower_copy(vm["motor"].as<std::string>()),
        vm["host"].as<std::string>(),
        vm["port"].as<std::string>(),
        vm["user"].as<std::string>(),
        vm["password"].as<std::string>(),
        vm["dbname"].as<std::string>(),
        vm["jwt-secret"].as<std::string>());
    ejecutarComando("cd " + dir_salida + " && npm install");
    imprimirRutasApi(esquema, dir_salida);
    ejecutarComando("cd " + dir_salida + " && npm run start:dev");
}

void manejarAuditoria(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion) {
    auto gestor_auditoria = std::make_shared<GestorAuditoria>(motor, info_conexion, vm["dbname"].as<std::string>());
    if (!gestor_auditoria->estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

    if (motor == GestorAuditoria::MotorDB::SQLite && vm.count("key")) {
        auto gestor_cifrado = std::make_shared<GestorCifrado>(gestor_auditoria, vm["key"].as<std::string>());
        gestor_auditoria->setGestorCifrado(gestor_cifrado);
    }

    std::vector<std::string> tablas = vm.count("tabla") ?
        std::vector<std::string>{vm["tabla"].as<std::string>()} :
        gestor_auditoria->obtenerNombresDeTablas(false);

    for (const auto& tabla : tablas) {
        std::cout << "Generando auditoria para: " << tabla << std::endl;
        gestor_auditoria->generarAuditoriaParaTabla(tabla);
    }
    std::cout << "Proceso de auditoria completado." << std::endl;
}

void manejarEncriptado(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion) {
    if (!vm.count("key")) throw std::runtime_error("--key es obligatorio para cualquier operacion de cifrado.");

    auto gestor_db = std::make_shared<GestorAuditoria>(motor, info_conexion, vm["dbname"].as<std::string>());
    if (!gestor_db->estaConectado()) throw std::runtime_error("No se pudo conectar a la base de datos.");

    GestorCifrado gestor_cifrado(gestor_db, vm["key"].as<std::string>());

    if (vm.count("encrypt-audit-tables")) {
        gestor_cifrado.cifrarTablasDeAuditoria();
    }
    else if (vm.count("query")) {
        std::string query = vm["query"].as<std::string>();
        std::cout << "Ejecutando consulta..." << std::endl;

        auto resultados = gestor_cifrado.ejecutarConsultaConDesencriptado(query);

        for (const auto& fila : resultados) {
            bool first = true;
            for (const auto& celda : fila) {
                if (!first) std::cout << "\t|\t";
                std::cout << celda;
                first = false;
            }
            std::cout << std::endl;
        }
    }
    else {
        throw std::runtime_error("La accion de encriptado requiere --encrypt-audit-tables o --query.");
    }
}