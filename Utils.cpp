#include "Utils.hpp"
#include <iostream>
#include <cstdlib>

void ejecutarComando(const std::string& comando, bool esperar) {
    std::cout << "Ejecutando comando: " << comando << std::endl;
    std::string comando_a_ejecutar = comando;
#ifdef _WIN32
    if (!esperar) {
        comando_a_ejecutar = "start \"NestJS API\" cmd /c \"" + comando + "\"";
    }
#else
    if (!esperar) {
        comando_a_ejecutar = comando + " &";
    }
#endif
    int resultado = system(comando_a_ejecutar.c_str());
    if (resultado != 0) {
        std::cerr << "Advertencia: El comando anterior finalizo con un codigo de error: " << resultado << std::endl;
    }
}

void imprimirRutasApi(const std::vector<Tabla>& tablas, const std::string& dir_salida) {
    std::cout << "\n--- Rutas de la API Generadas ---" << std::endl;
    std::cout << "URL Base: http://localhost:3000\n" << std::endl;

    for (const auto& tabla : tablas) {
        if (tabla.es_tabla_usuario) {
            std::cout << "Autenticacion y Registro de Usuarios:" << std::endl;
            std::cout << "  POST   /autenticacion/login     (Requiere {" << tabla.campo_email_encontrado << ", " << tabla.campo_contrasena_encontrado << "})" << std::endl;
            break;
        }
    }

    std::cout << "\nEndpoints CRUD Generados:" << std::endl;
    for (const auto& tabla : tablas) {
        std::string ruta = "/" + tabla.nombre_archivo;
        std::string proteccion_info = tabla.es_protegida ? " (Protegida con JWT)" : " (Ruta Publica)";
        std::cout << "Recurso: " << tabla.nombre_clase << proteccion_info << std::endl;
        std::cout << "  POST   " << ruta << "                (Crear nuevo)" << std::endl;
        std::cout << "  GET    " << ruta << "                (Obtener todos)" << std::endl;
        std::cout << "  GET    " << ruta << "/:id" << "                 (Obtener por ID)" << std::endl;
        std::cout << "  PATCH  " << ruta << "/:id" << "                 (Actualizar por ID)" << std::endl;
        std::cout << "  DELETE " << ruta << "/:id" << "                 (Eliminar por ID)" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
    }
}