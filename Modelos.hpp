#pragma once
#include <string>
#include <vector>

struct Columna {
    std::string nombre;
    std::string nombre_camel_case;
    std::string tipo_db;
    std::string tipo_ts;
    bool es_nulo;
    bool es_pk = false;
};

struct Tabla {
    std::string nombre;
    std::string nombre_clase;
    std::string nombre_variable;
    std::string nombre_archivo;
    Columna clave_primaria;
    std::vector<Columna> columnas;
    std::vector<std::string> dependencias_fk;
    bool es_tabla_usuario = false;
    bool es_protegida = true;
    std::string campo_email_encontrado;
    std::string campo_contrasena_encontrado;
};