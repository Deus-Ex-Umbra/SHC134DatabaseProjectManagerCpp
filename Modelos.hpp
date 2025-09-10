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
    bool es_fk = false;
};

struct DependenciaFK {
    std::string columna_local;
    std::string tabla_referenciada;
    std::string clase_tabla_referenciada;
    std::string variable_tabla_referenciada;
};

struct Tabla {
    std::string nombre;
    std::string nombre_clase;
    std::string nombre_variable;
    std::string nombre_archivo;
    Columna clave_primaria;
    std::vector<Columna> columnas;
    std::vector<DependenciaFK> dependencias_fk;
    bool es_tabla_usuario = false;
    bool es_protegida = true;
    std::string campo_email_encontrado;
    std::string campo_contrasena_encontrado;
};