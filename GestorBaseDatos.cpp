#include "GestorBaseDatos.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <unordered_map>

GestorBaseDatos::GestorBaseDatos(const std::string& info_conexion) {
    conexion = PQconnectdb(info_conexion.c_str());
    if (PQstatus(conexion) != CONNECTION_OK) {
        std::cerr << "Error Crítico: No se pudo conectar a la base de datos: " << PQerrorMessage(conexion) << std::endl;
        PQfinish(conexion);
        conexion = nullptr;
    }
}

GestorBaseDatos::~GestorBaseDatos() {
    if (conexion) {
        PQfinish(conexion);
    }
}

bool GestorBaseDatos::estaConectado() {
    return conexion != nullptr;
}

std::vector<std::string> GestorBaseDatos::obtenerNombresDeTablas() {
    std::vector<std::string> tablas;
    const char* consulta = "SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname = 'public';";
    PGresult* res = PQexec(conexion, consulta);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Fallo en la consulta de nombres de tablas: " << PQerrorMessage(conexion) << std::endl;
        PQclear(res);
        return tablas;
    }
    for (int i = 0; i < PQntuples(res); i++) {
        tablas.push_back(PQgetvalue(res, i, 0));
    }
    PQclear(res);
    return tablas;
}

std::vector<Columna> GestorBaseDatos::obtenerColumnasParaTabla(const std::string& nombre_tabla) {
    std::vector<Columna> columnas;
    std::string consulta_str =
        "SELECT c.column_name, c.udt_name, c.is_nullable, "
        " (SELECT count(kcu.column_name) "
        "  FROM information_schema.key_column_usage AS kcu "
        "  JOIN information_schema.table_constraints AS tc ON kcu.constraint_name = tc.constraint_name "
        "  WHERE kcu.table_name=c.table_name AND kcu.column_name=c.column_name AND tc.constraint_type='PRIMARY KEY') as es_pk "
        "FROM information_schema.columns AS c "
        "WHERE c.table_schema = 'public' AND c.table_name = '" + nombre_tabla + "' ORDER BY c.ordinal_position;";

    PGresult* res = PQexec(conexion, consulta_str.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Fallo en la consulta de columnas para la tabla " << nombre_tabla << ": " << PQerrorMessage(conexion) << std::endl;
        PQclear(res);
        return columnas;
    }

    for (int i = 0; i < PQntuples(res); i++) {
        Columna col;
        col.nombre = PQgetvalue(res, i, 0);
        col.nombre_camel_case = aCamelCase(col.nombre);
        col.tipo_db = PQgetvalue(res, i, 1);
        col.tipo_ts = mapearTipoDbATs(col.tipo_db);
        col.es_nulo = (std::string(PQgetvalue(res, i, 2)) == "YES");
        col.es_pk = (std::string(PQgetvalue(res, i, 3)) == "1");
        columnas.push_back(col);
    }
    PQclear(res);
    return columnas;
}

void GestorBaseDatos::obtenerDependenciasFk(std::vector<Tabla>& tablas) {
    std::unordered_map<std::string, Tabla*> mapa_tablas;
    for (auto& tabla : tablas) {
        mapa_tablas[tabla.nombre] = &tabla;
    }

    const char* consulta =
        "SELECT "
        "  tc.table_name, ccu.table_name AS foreign_table_name "
        "FROM "
        "  information_schema.table_constraints AS tc "
        "  JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema "
        "  JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name AND ccu.table_schema = tc.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_schema = 'public';";

    PGresult* res = PQexec(conexion, consulta);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Fallo en la consulta de claves foráneas: " << PQerrorMessage(conexion) << std::endl;
        PQclear(res);
        return;
    }

    for (int i = 0; i < PQntuples(res); i++) {
        std::string tabla_origen = PQgetvalue(res, i, 0);
        std::string tabla_dependencia = PQgetvalue(res, i, 1);
        if (mapa_tablas.count(tabla_origen)) {
            mapa_tablas[tabla_origen]->dependencias_fk.push_back(tabla_dependencia);
        }
    }
    PQclear(res);
}


void GestorBaseDatos::analizarDependenciasParaJwt(std::vector<Tabla>& tablas) {
    const Tabla* tabla_usuario = nullptr;
    for (const auto& tabla : tablas) {
        if (tabla.es_tabla_usuario) {
            tabla_usuario = &tabla;
            break;
        }
    }

    if (!tabla_usuario) {
        std::cout << "Advertencia: No se encontró una tabla de usuario. Todas las rutas serán protegidas por defecto." << std::endl;
        return;
    }

    std::unordered_set<std::string> dependencias_a_liberar;
    std::vector<std::string> a_visitar;

    a_visitar.push_back(tabla_usuario->nombre);

    std::unordered_map<std::string, const Tabla*> mapa_tablas;
    for (const auto& tabla : tablas) {
        mapa_tablas[tabla.nombre] = &tabla;
    }

    while (!a_visitar.empty()) {
        std::string nombre_tabla_actual = a_visitar.back();
        a_visitar.pop_back();

        if (dependencias_a_liberar.find(nombre_tabla_actual) == dependencias_a_liberar.end()) {
            dependencias_a_liberar.insert(nombre_tabla_actual);
            if (mapa_tablas.count(nombre_tabla_actual)) {
                const Tabla* tabla_actual = mapa_tablas.at(nombre_tabla_actual);
                for (const auto& nueva_dependencia : tabla_actual->dependencias_fk) {
                    a_visitar.push_back(nueva_dependencia);
                }
            }
        }
    }

    for (auto& tabla : tablas) {
        if (dependencias_a_liberar.count(tabla.nombre)) {
            tabla.es_protegida = false;
        }
    }
    std::cout << "Análisis de dependencias JWT completado. Rutas públicas: ";
    for (const auto& nombre_tabla : dependencias_a_liberar) {
        std::cout << nombre_tabla << " ";
    }
    std::cout << std::endl;
}

std::vector<Tabla> GestorBaseDatos::obtenerEsquemaTablas() {
    std::vector<Tabla> esquema_completo;
    std::vector<std::string> nombres_tablas = obtenerNombresDeTablas();

    const std::vector<std::string> posibles_tablas_usuario = { "user", "users", "usuario", "usuarios" };
    const std::vector<std::string> posibles_campos_email = { "email", "correo", "correo_electronico", "nombre_email", "nombreemail" };
    const std::vector<std::string> posibles_campos_contrasena = { "password", "contrasena", "contrasenia", "clave" };

    for (auto& nombre : nombres_tablas) {
        Tabla tabla;
        tabla.nombre = nombre;
        tabla.nombre_clase = aPascalCase(nombre);
        tabla.nombre_variable = aCamelCase(tabla.nombre_clase);
        tabla.nombre_archivo = aKebabCase(tabla.nombre_clase);
        tabla.columnas = obtenerColumnasParaTabla(nombre);

        bool pk_encontrada = false;
        for (const auto& col : tabla.columnas) {
            if (col.es_pk) {
                tabla.clave_primaria = col;
                pk_encontrada = true;
                break;
            }
        }
        if (!pk_encontrada && !tabla.columnas.empty()) {
            tabla.clave_primaria = tabla.columnas[0];
        }

        std::string nombre_lower = nombre;
        std::transform(nombre_lower.begin(), nombre_lower.end(), nombre_lower.begin(), ::tolower);
        if (std::find(posibles_tablas_usuario.begin(), posibles_tablas_usuario.end(), nombre_lower) != posibles_tablas_usuario.end()) {
            tabla.es_tabla_usuario = true;
            for (const auto& col : tabla.columnas) {
                std::string nombre_col_lower = col.nombre;
                std::transform(nombre_col_lower.begin(), nombre_col_lower.end(), nombre_col_lower.begin(), ::tolower);

                if (tabla.campo_email_encontrado.empty() &&
                    std::find(posibles_campos_email.begin(), posibles_campos_email.end(), nombre_col_lower) != posibles_campos_email.end()) {
                    tabla.campo_email_encontrado = col.nombre_camel_case;
                }
                if (tabla.campo_contrasena_encontrado.empty() &&
                    std::find(posibles_campos_contrasena.begin(), posibles_campos_contrasena.end(), nombre_col_lower) != posibles_campos_contrasena.end()) {
                    tabla.campo_contrasena_encontrado = col.nombre_camel_case;
                }
            }
            if (tabla.campo_email_encontrado.empty() || tabla.campo_contrasena_encontrado.empty()) {
                std::cerr << "ADVERTENCIA: En la tabla de usuario '" << tabla.nombre << "', no se pudo encontrar un campo estándar para email o contraseña. La autenticación podría no funcionar como se espera." << std::endl;
            }
        }

        esquema_completo.push_back(tabla);
    }

    obtenerDependenciasFk(esquema_completo);
    analizarDependenciasParaJwt(esquema_completo);

    return esquema_completo;
}

std::string GestorBaseDatos::aPascalCase(const std::string& entrada) {
    std::string resultado = "";
    bool capitalizar = true;
    for (char c : entrada) {
        if (c == '_') {
            capitalizar = true;
        }
        else {
            if (capitalizar) {
                resultado += toupper(c);
                capitalizar = false;
            }
            else {
                resultado += c;
            }
        }
    }
    if (resultado.length() > 2 && resultado.substr(resultado.length() - 2) == "es") {
        resultado.erase(resultado.length() - 2);
    }
    else if (resultado.length() > 1 && resultado.back() == 's') {
        resultado.pop_back();
    }
    return resultado;
}

std::string GestorBaseDatos::aCamelCase(const std::string& entrada) {
    std::string pascal_case = aPascalCase(entrada);
    if (!pascal_case.empty()) {
        pascal_case[0] = tolower(pascal_case[0]);
    }
    return pascal_case;
}

std::string GestorBaseDatos::aKebabCase(const std::string& entradaPascalCase) {
    std::string resultado = "";
    if (!entradaPascalCase.empty()) {
        resultado += tolower(entradaPascalCase[0]);
        for (size_t i = 1; i < entradaPascalCase.length(); ++i) {
            if (isupper(entradaPascalCase[i])) {
                resultado += '-';
            }
            resultado += tolower(entradaPascalCase[i]);
        }
    }
    return resultado;
}

std::string GestorBaseDatos::mapearTipoDbATs(const std::string& tipo_db) {
    if (tipo_db.find("char") != std::string::npos || tipo_db.find("text") != std::string::npos) return "string";
    if (tipo_db.find("int") != std::string::npos || tipo_db.find("serial") != std::string::npos || tipo_db.find("numeric") != std::string::npos || tipo_db.find("decimal") != std::string::npos || tipo_db.find("float") != std::string::npos || tipo_db.find("double") != std::string::npos || tipo_db.find("real") != std::string::npos) return "number";
    if (tipo_db.find("bool") != std::string::npos) return "boolean";
    if (tipo_db.find("date") != std::string::npos || tipo_db.find("time") != std::string::npos) return "Date";
    if (tipo_db.find("json") != std::string::npos) return "any";
    return "any";
}