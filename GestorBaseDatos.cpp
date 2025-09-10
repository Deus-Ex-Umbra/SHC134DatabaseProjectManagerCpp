#include "GestorBaseDatos.hpp"
#include "Utils.hpp" 
#include <iostream>
#include <algorithm>
#include <cctype>
#include <boost/algorithm/string.hpp>

GestorBaseDatos::GestorBaseDatos(GestorAuditoria::MotorDB motor, const std::string& info_conexion, const std::string& dbname)
    : motor_actual(motor), db_name(dbname) {
    try {
        switch (motor_actual) {
        case GestorAuditoria::MotorDB::PostgreSQL:
            conn_pg = PQconnectdb(info_conexion.c_str());
            if (PQstatus(conn_pg) != CONNECTION_OK) {
                throw std::runtime_error(PQerrorMessage(conn_pg));
            }
            break;
        case GestorAuditoria::MotorDB::SQLServer:
        case GestorAuditoria::MotorDB::MySQL:
        case GestorAuditoria::MotorDB::SQLite:
            conn_odbc = std::make_unique<nanodbc::connection>(NANODBC_TEXT(info_conexion));
            break;
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error de conexion: " + std::string(e.what()));
    }
}

GestorBaseDatos::~GestorBaseDatos() {
    if (conn_pg) {
        PQfinish(conn_pg);
    }
    if (conn_odbc && conn_odbc->connected()) {
        conn_odbc->disconnect();
    }
}

bool GestorBaseDatos::estaConectado() {
    switch (motor_actual) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        return conn_pg && PQstatus(conn_pg) == CONNECTION_OK;
    default:
        return conn_odbc && conn_odbc->connected();
    }
}

std::string GestorBaseDatos::mapearTipoDbATs(const std::string& tipo_db) {
    std::string tipo_lower = tipo_db;
    boost::to_lower(tipo_lower);

    if (tipo_lower.find("int") != std::string::npos ||
        tipo_lower.find("serial") != std::string::npos ||
        tipo_lower.find("bigint") != std::string::npos ||
        tipo_lower.find("smallint") != std::string::npos ||
        tipo_lower.find("numeric") != std::string::npos ||
        tipo_lower.find("decimal") != std::string::npos ||
        tipo_lower.find("real") != std::string::npos ||
        tipo_lower.find("float") != std::string::npos ||
        tipo_lower.find("double") != std::string::npos) {
        return "number";
    }

    if (tipo_lower.find("bool") != std::string::npos) {
        return "boolean";
    }

    if (tipo_lower.find("date") != std::string::npos ||
        tipo_lower.find("time") != std::string::npos) {
        return "Date";
    }

    return "string";
}

std::vector<std::string> GestorBaseDatos::obtenerNombresDeTablas() {
    std::vector<std::string> tablas;
    std::string consulta;

    switch (motor_actual) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        consulta = "SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname = 'public' ORDER BY tablename;";
        break;
    case GestorAuditoria::MotorDB::MySQL:
        consulta = "SELECT table_name FROM information_schema.tables WHERE table_schema = DATABASE() ORDER BY table_name;";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        consulta = "SELECT name FROM sys.tables ORDER BY name;";
        break;
    case GestorAuditoria::MotorDB::SQLite:
        consulta = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
        break;
    }

    if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
        PGresult* res = PQexec(conn_pg, consulta.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            for (int i = 0; i < PQntuples(res); ++i) {
                std::string nombre_tabla = PQgetvalue(res, i, 0);
                if (nombre_tabla.rfind("aud_", 0) != 0 && nombre_tabla != "sysdiagrams") {
                    tablas.push_back(nombre_tabla);
                }
            }
        }
        PQclear(res);
    }
    else {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        while (res.next()) {
            std::string nombre_tabla = res.get<std::string>(0);
            if (nombre_tabla.rfind("aud_", 0) != 0 && nombre_tabla != "sysdiagrams" &&
                nombre_tabla.rfind("sqlite_", 0) != 0) {
                tablas.push_back(nombre_tabla);
            }
        }
    }

    return tablas;
}

std::vector<Columna> GestorBaseDatos::obtenerColumnasParaTabla(const std::string& nombre_tabla) {
    std::vector<Columna> columnas;
    std::string consulta;
    std::string pk_col_name;

    switch (motor_actual) {
    case GestorAuditoria::MotorDB::PostgreSQL: {
        std::string pk_query = "SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '" + nombre_tabla + "';";
        PGresult* pk_res = PQexec(conn_pg, pk_query.c_str());
        if (PQntuples(pk_res) > 0) pk_col_name = PQgetvalue(pk_res, 0, 0);
        PQclear(pk_res);
        consulta = "SELECT column_name, data_type, is_nullable FROM information_schema.columns WHERE table_name = '" + nombre_tabla + "' ORDER BY ordinal_position;";
        break;
    }
    case GestorAuditoria::MotorDB::MySQL:
        consulta = "SELECT column_name, data_type, is_nullable, column_key FROM information_schema.columns WHERE table_name = '" + nombre_tabla + "' AND table_schema = DATABASE() ORDER BY ordinal_position;";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        consulta = "SELECT c.name, t.name, c.is_nullable, ISNULL(i.is_primary_key, 0) FROM sys.columns c INNER JOIN sys.types t ON c.user_type_id = t.user_type_id LEFT JOIN sys.index_columns ic ON ic.object_id = c.object_id AND ic.column_id = c.column_id LEFT JOIN sys.indexes i ON i.object_id = ic.object_id AND i.index_id = ic.index_id AND i.is_primary_key = 1 WHERE c.object_id = OBJECT_ID('" + nombre_tabla + "') ORDER BY c.column_id;";
        break;
    case GestorAuditoria::MotorDB::SQLite:
        consulta = "PRAGMA table_info(" + nombre_tabla + ");";
        break;
    }

    if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
        PGresult* res = PQexec(conn_pg, consulta.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            for (int i = 0; i < PQntuples(res); ++i) {
                Columna col;
                col.nombre = PQgetvalue(res, i, 0);
                col.tipo_db = PQgetvalue(res, i, 1);
                col.tipo_ts = mapearTipoDbATs(col.tipo_db);
                col.es_nulo = std::string(PQgetvalue(res, i, 2)) == "YES";
                col.es_pk = col.nombre == pk_col_name;
                columnas.push_back(col);
            }
        }
        PQclear(res);
    }
    else if (motor_actual == GestorAuditoria::MotorDB::SQLite) {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        while (res.next()) {
            Columna col;
            col.nombre = res.get<std::string>(1);
            col.tipo_db = res.get<std::string>(2);
            col.tipo_ts = mapearTipoDbATs(col.tipo_db);
            col.es_nulo = res.get<int>(3) == 0;
            col.es_pk = res.get<int>(5) == 1;
            columnas.push_back(col);
        }
    }
    else {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        while (res.next()) {
            Columna col;
            col.nombre = res.get<std::string>(0);
            col.tipo_db = res.get<std::string>(1);
            col.tipo_ts = mapearTipoDbATs(col.tipo_db);
            col.es_nulo = res.get<std::string>(2) == "YES";
            if (motor_actual == GestorAuditoria::MotorDB::MySQL) {
                col.es_pk = res.get<std::string>(3) == "PRI";
            }
            else {
                col.es_pk = res.get<int>(3) == 1;
            }
            columnas.push_back(col);
        }
    }
    return columnas;
}

void GestorBaseDatos::obtenerDependenciasFk(std::vector<Tabla>& tablas) {
    for (auto& tabla : tablas) {
        std::string consulta;
        switch (motor_actual) {
        case GestorAuditoria::MotorDB::PostgreSQL:
            consulta = "SELECT kcu.column_name, ccu.table_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name JOIN information_schema.constraint_column_usage ccu ON ccu.constraint_name = tc.constraint_name WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = '" + tabla.nombre + "';";
            break;
        case GestorAuditoria::MotorDB::MySQL:
            consulta = "SELECT kcu.column_name, kcu.referenced_table_name FROM information_schema.key_column_usage kcu WHERE kcu.table_name = '" + tabla.nombre + "' AND kcu.table_schema = DATABASE() AND kcu.referenced_table_name IS NOT NULL;";
            break;
        case GestorAuditoria::MotorDB::SQLServer:
            consulta = "SELECT col.name, ref_tab.name FROM sys.foreign_key_columns fk INNER JOIN sys.columns col ON fk.parent_object_id = col.object_id AND fk.parent_column_id = col.column_id INNER JOIN sys.tables tab ON fk.parent_object_id = tab.object_id INNER JOIN sys.tables ref_tab ON fk.referenced_object_id = ref_tab.object_id WHERE tab.name = '" + tabla.nombre + "';";
            break;
        default:
            continue;
        }

        auto procesar_fk = [&](const std::string& col_local, const std::string& tabla_ref) {
            tabla.dependencias_fk.push_back({ col_local, tabla_ref });
            for (auto& col : tabla.columnas) {
                if (col.nombre == col_local) {
                    col.es_fk = true;
                    break;
                }
            }
            };

        if (motor_actual == GestorAuditoria::MotorDB::PostgreSQL) {
            PGresult* res = PQexec(conn_pg, consulta.c_str());
            if (PQresultStatus(res) == PGRES_TUPLES_OK) {
                for (int i = 0; i < PQntuples(res); ++i) {
                    procesar_fk(PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
                }
            }
            PQclear(res);
        }
        else {
            nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
            while (res.next()) {
                procesar_fk(res.get<std::string>(0), res.get<std::string>(1));
            }
        }
    }
}

void GestorBaseDatos::analizarDependenciasParaJwt(std::vector<Tabla>& tablas) {
    Tabla* tabla_usuario = nullptr;
    for (auto& tabla : tablas) {
        std::string nombre_lower = boost::to_lower_copy(tabla.nombre);
        if (nombre_lower.find("user") != std::string::npos || nombre_lower.find("usuario") != std::string::npos) {
            for (const auto& col : tabla.columnas) {
                std::string col_lower = boost::to_lower_copy(col.nombre);
                if (tabla.campo_email_encontrado.empty() && (col_lower.find("email") != std::string::npos || col_lower.find("correo") != std::string::npos || col_lower.find("username") != std::string::npos)) {
                    tabla.campo_email_encontrado = col.nombre;
                }
                if (tabla.campo_contrasena_encontrado.empty() && (col_lower.find("password") != std::string::npos || col_lower.find("contrasena") != std::string::npos)) {
                    tabla.campo_contrasena_encontrado = col.nombre;
                }
            }
            if (!tabla.campo_email_encontrado.empty() && !tabla.campo_contrasena_encontrado.empty()) {
                tabla.es_tabla_usuario = true;
                tabla_usuario = &tabla;
                break;
            }
        }
    }

    if (tabla_usuario) {
        std::vector<std::string> tablas_desprotegidas = { tabla_usuario->nombre };
        std::function<void(const std::string&)> agregar_dependencias =
            [&](const std::string& nombre_tabla) {
            for (const auto& t : tablas) {
                if (t.nombre == nombre_tabla) {
                    for (const auto& dep : t.dependencias_fk) {
                        if (std::find(tablas_desprotegidas.begin(), tablas_desprotegidas.end(), dep.tabla_referenciada) == tablas_desprotegidas.end()) {
                            tablas_desprotegidas.push_back(dep.tabla_referenciada);
                            agregar_dependencias(dep.tabla_referenciada);
                        }
                    }
                }
            }
            };
        agregar_dependencias(tabla_usuario->nombre);

        for (auto& tabla : tablas) {
            if (std::find(tablas_desprotegidas.begin(), tablas_desprotegidas.end(), tabla.nombre) == tablas_desprotegidas.end()) {
                tabla.es_protegida = true;
            }
            else {
                tabla.es_protegida = false;
            }
        }
    }
}

std::vector<Tabla> GestorBaseDatos::obtenerEsquemaTablas() {
    std::vector<Tabla> tablas;
    std::vector<std::string> nombres_tablas = obtenerNombresDeTablas();

    for (const auto& nombre_tabla : nombres_tablas) {
        Tabla tabla;
        tabla.nombre = nombre_tabla;
        tabla.nombre_clase = aPascalCase(nombre_tabla);
        tabla.nombre_variable = aCamelCase(nombre_tabla);
        tabla.nombre_archivo = aKebabCase(tabla.nombre_clase);
        tabla.columnas = obtenerColumnasParaTabla(nombre_tabla);
        for (const auto& col : tabla.columnas) {
            if (col.es_pk) {
                tabla.clave_primaria = col;
                break;
            }
        }
        tablas.push_back(tabla);
    }

    obtenerDependenciasFk(tablas);
    analizarDependenciasParaJwt(tablas);
    return tablas;
}