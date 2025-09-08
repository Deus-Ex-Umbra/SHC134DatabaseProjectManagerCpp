#include "GestorBaseDatos.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <unordered_map>

GestorBaseDatos::GestorBaseDatos(GestorAuditoria::MotorDB motor, const std::string& info_conexion, const std::string& dbname)
    : motor_actual(motor), db_name(dbname) {
    try {
        if (motor == GestorAuditoria::MotorDB::PostgreSQL) {
            conn_pg = PQconnectdb(info_conexion.c_str());
            if (PQstatus(conn_pg) != CONNECTION_OK) {
                throw std::runtime_error(PQerrorMessage(conn_pg));
            }
        }
        else {
            conn_odbc = std::make_unique<nanodbc::connection>(NANODBC_TEXT(info_conexion));
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error de conexion en GestorBaseDatos: " + std::string(e.what()));
    }
}

GestorBaseDatos::~GestorBaseDatos() {
    if (conn_pg) PQfinish(conn_pg);
    if (conn_odbc && conn_odbc->connected()) conn_odbc->disconnect();
}

bool GestorBaseDatos::estaConectado() {
    if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
        return conn_pg && PQstatus(conn_pg) == CONNECTION_OK;
    }
    return conn_odbc && conn_odbc->connected();
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
    if (tipo_db.find("char") != std::string::npos || tipo_db.find("text") != std::string::npos || tipo_db.find("varchar") != std::string::npos) return "string";
    if (tipo_db.find("int") != std::string::npos || tipo_db.find("serial") != std::string::npos || tipo_db.find("numeric") != std::string::npos || tipo_db.find("decimal") != std::string::npos || tipo_db.find("float") != std::string::npos || tipo_db.find("double") != std::string::npos || tipo_db.find("real") != std::string::npos) return "number";
    if (tipo_db.find("bool") != std::string::npos) return "boolean";
    if (tipo_db.find("date") != std::string::npos || tipo_db.find("time") != std::string::npos) return "Date";
    if (tipo_db.find("json") != std::string::npos) return "any";
    return "any";
}

std::vector<std::string> GestorBaseDatos::obtenerNombresDeTablas() {
    std::vector<std::string> tablas;
    std::string consulta;

    switch (motor_actual) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        consulta = "SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname = 'public';";
        break;
    case GestorAuditoria::MotorDB::MySQL:
        consulta = "SELECT table_name FROM information_schema.tables WHERE table_schema = '" + db_name + "';";
        break;
    case GestorAuditoria::MotorDB::SQLite:
        consulta = "SELECT name FROM sqlite_master WHERE type='table';";
        break;
    default:
        return tablas;
    }

    if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
        PGresult* res = PQexec(conn_pg, consulta.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            for (int i = 0; i < PQntuples(res); ++i) tablas.push_back(PQgetvalue(res, i, 0));
        }
        PQclear(res);
    }
    else {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        while (res.next()) tablas.push_back(res.get<std::string>(0));
    }

    tablas.erase(std::remove_if(tablas.begin(), tablas.end(), [](const std::string& s) {
        return s.rfind("sqlite_") == 0 || s.rfind("aud_") == 0 || s.rfind("Aud") == 0;
        }), tablas.end());

    return tablas;
}

std::vector<Columna> GestorBaseDatos::obtenerColumnasParaTabla(const std::string& nombre_tabla) {
    std::vector<Columna> columnas;
    if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
        std::string consulta_str = "SELECT c.column_name, c.udt_name, c.is_nullable, "
            "(SELECT count(kcu.column_name) FROM information_schema.key_column_usage AS kcu JOIN information_schema.table_constraints AS tc ON kcu.constraint_name = tc.constraint_name "
            "WHERE kcu.table_name=c.table_name AND kcu.column_name=c.column_name AND tc.constraint_type='PRIMARY KEY') as es_pk "
            "FROM information_schema.columns AS c WHERE c.table_schema = 'public' AND c.table_name = '" + nombre_tabla + "' ORDER BY c.ordinal_position;";

        PGresult* res = PQexec(conn_pg, consulta_str.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            for (int i = 0; i < PQntuples(res); i++) {
                Columna col;
                col.nombre = PQgetvalue(res, i, 0);
                col.tipo_db = PQgetvalue(res, i, 1);
                col.es_nulo = (std::string(PQgetvalue(res, i, 2)) == "YES");
                col.es_pk = (std::string(PQgetvalue(res, i, 3)) == "1");
                col.nombre_camel_case = aCamelCase(col.nombre);
                col.tipo_ts = mapearTipoDbATs(col.tipo_db);
                columnas.push_back(col);
            }
        }
        PQclear(res);
    }
    else if (motor_actual == GestorAuditoria::MotorDB::MySQL) {
        std::string consulta_str = "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE, (CASE WHEN COLUMN_KEY = 'PRI' THEN 1 ELSE 0 END) as IS_PK FROM information_schema.COLUMNS "
            "WHERE TABLE_SCHEMA = '" + db_name + "' AND TABLE_NAME = '" + nombre_tabla + "' ORDER BY ORDINAL_POSITION;";
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta_str));
        while (res.next()) {
            Columna col;
            col.nombre = res.get<std::string>(0);
            col.tipo_db = res.get<std::string>(1);
            col.es_nulo = (res.get<std::string>(2) == "YES");
            col.es_pk = (res.get<int>(3) == 1);
            col.nombre_camel_case = aCamelCase(col.nombre);
            col.tipo_ts = mapearTipoDbATs(col.tipo_db);
            columnas.push_back(col);
        }
    }
    else if (motor_actual == GestorAuditoria::MotorDB::SQLite) {
        std::string consulta_str = "PRAGMA table_info(" + nombre_tabla + ");";
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta_str));
        while (res.next()) {
            Columna col;
            col.nombre = res.get<std::string>(1);
            col.tipo_db = res.get<std::string>(2);
            col.es_nulo = (res.get<int>(3) == 0);
            col.es_pk = (res.get<int>(5) == 1);
            col.nombre_camel_case = aCamelCase(col.nombre);
            col.tipo_ts = mapearTipoDbATs(col.tipo_db);
            columnas.push_back(col);
        }
    }
    return columnas;
}

void GestorBaseDatos::obtenerDependenciasFk(std::vector<Tabla>& tablas) {
    // Implementación pendiente para múltiples bases de datos
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
}

std::vector<Tabla> GestorBaseDatos::obtenerEsquemaTablas() {
    std::vector<Tabla> esquema_completo;
    std::vector<std::string> nombres_tablas = obtenerNombresDeTablas();
    const std::vector<std::string> posibles_tablas_usuario = { "user", "users", "usuario", "usuarios" };
    const std::vector<std::string> posibles_campos_email = { "email", "correo", "correo_electronico" };
    const std::vector<std::string> posibles_campos_contrasena = { "password", "contrasena", "clave" };

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
                if (tabla.campo_email_encontrado.empty() && std::find(posibles_campos_email.begin(), posibles_campos_email.end(), nombre_col_lower) != posibles_campos_email.end()) {
                    tabla.campo_email_encontrado = col.nombre_camel_case;
                }
                if (tabla.campo_contrasena_encontrado.empty() && std::find(posibles_campos_contrasena.begin(), posibles_campos_contrasena.end(), nombre_col_lower) != posibles_campos_contrasena.end()) {
                    tabla.campo_contrasena_encontrado = col.nombre_camel_case;
                }
            }
        }
        esquema_completo.push_back(tabla);
    }

    obtenerDependenciasFk(esquema_completo);
    analizarDependenciasParaJwt(esquema_completo);
    return esquema_completo;
}