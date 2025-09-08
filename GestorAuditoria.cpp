#include "GestorAuditoria.hpp"
#include <algorithm>
#include <iostream>

GestorAuditoria::GestorAuditoria(MotorDB motor, const std::string& connection_string, const std::string& db)
    : motor_actual(motor), db_name(db) {
    conectar(connection_string, db);
    if (estaConectado()) {
        crearFuncionesAuditoria();
    }
}

GestorAuditoria::~GestorAuditoria() {
    desconectar();
}

void GestorAuditoria::conectar(const std::string& connection_string, const std::string& db) {
    try {
        switch (motor_actual) {
        case MotorDB::PostgreSQL:
            conn_pg = PQconnectdb(connection_string.c_str());
            if (PQstatus(conn_pg) != CONNECTION_OK) throw std::runtime_error(PQerrorMessage(conn_pg));
            break;
        case MotorDB::MySQL:
            conn_mysql = std::make_unique<mysqlx::Session>(connection_string);
            mysql_conectado = true;
            break;
        case MotorDB::SQLServer:
        case MotorDB::SQLite:
            conn_odbc = std::make_unique<nanodbc::connection>(NANODBC_TEXT(connection_string));
            break;
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error de conexion: " + std::string(e.what()));
    }
}

void GestorAuditoria::desconectar() {
    if (conn_pg) PQfinish(conn_pg);
    if (conn_mysql) {
        conn_mysql->close();
        mysql_conectado = false;
    }
    if (conn_odbc && conn_odbc->connected()) conn_odbc->disconnect();
}

bool GestorAuditoria::estaConectado() const {
    switch (motor_actual) {
    case MotorDB::PostgreSQL: return conn_pg && PQstatus(conn_pg) == CONNECTION_OK;
    case MotorDB::MySQL: return mysql_conectado;
    default: return conn_odbc && conn_odbc->connected();
    }
}

void GestorAuditoria::ejecutarComando(const std::string& consulta) {
    try {
        switch (motor_actual) {
        case MotorDB::PostgreSQL: {
            PGresult* res = PQexec(conn_pg, consulta.c_str());
            if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
                std::string error = PQerrorMessage(conn_pg);
                if (error.find("already exists") == std::string::npos && error.find("does not exist") == std::string::npos)
                    throw std::runtime_error(error);
            }
            PQclear(res);
            break;
        }
        case MotorDB::MySQL:
            conn_mysql->sql(consulta).execute();
            break;
        default:
            nanodbc::just_execute(*conn_odbc, NANODBC_TEXT(consulta));
            break;
        }
    }
    catch (const std::exception& e) {
        std::string error = e.what();
        if (error.find("already exists") == std::string::npos && error.find("does not exist") == std::string::npos) {
            throw;
        }
    }
}

std::vector<std::string> GestorAuditoria::obtenerNombresDeTablas(bool incluir_auditoria) {
    std::vector<std::string> tablas;
    std::string consulta;

    switch (motor_actual) {
    case MotorDB::PostgreSQL:
        consulta = "SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname = 'public';";
        break;
    case MotorDB::MySQL:
        consulta = "SELECT table_name FROM information_schema.tables WHERE table_schema = DATABASE();";
        break;
    case MotorDB::SQLServer:
        consulta = "SELECT name FROM sys.tables;";
        break;
    case MotorDB::SQLite:
        consulta = "SELECT name FROM sqlite_master WHERE type='table';";
        break;
    }

    if (motor_actual == MotorDB::PostgreSQL) {
        PGresult* res = PQexec(conn_pg, consulta.c_str());
        for (int i = 0; i < PQntuples(res); ++i) tablas.push_back(PQgetvalue(res, i, 0));
        PQclear(res);
    }
    else if (motor_actual == MotorDB::MySQL) {
        mysqlx::RowResult res = conn_mysql->sql(consulta).execute();
        for (mysqlx::Row row : res.fetchAll()) {
            tablas.push_back(row[0].get<std::string>());
        }
    }
    else {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        while (res.next()) tablas.push_back(res.get<std::string>(0));
    }

    if (!incluir_auditoria) {
        tablas.erase(std::remove_if(tablas.begin(), tablas.end(), [](const std::string& s) {
            return s.rfind("aud_", 0) == 0 || s.rfind("Aud", 0) == 0 || s == "sysdiagrams" || s.rfind("sqlite_", 0) == 0;
            }), tablas.end());
    }
    return tablas;
}

ResultadoConsulta GestorAuditoria::ejecutarConsultaConResultado(const std::string& consulta) {
    ResultadoConsulta resultado;
    switch (motor_actual) {
    case MotorDB::PostgreSQL: {
        PGresult* res = PQexec(conn_pg, consulta.c_str());
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            for (int j = 0; j < PQnfields(res); ++j) {
                resultado.columnas.push_back(PQfname(res, j));
            }
            for (int i = 0; i < PQntuples(res); ++i) {
                std::vector<std::string> fila;
                for (int j = 0; j < PQnfields(res); ++j) {
                    fila.push_back(PQgetisnull(res, i, j) ? "NULL" : PQgetvalue(res, i, j));
                }
                resultado.filas.push_back(fila);
            }
        }
        PQclear(res);
        break;
    }
    case MotorDB::MySQL: {
        mysqlx::RowResult res = conn_mysql->sql(consulta).execute();
        for (const auto& col : res.getColumns()) {
            resultado.columnas.push_back(col.getColumnName());
        }
        for (mysqlx::Row row : res.fetchAll()) {
            std::vector<std::string> fila;
            for (size_t i = 0; i < row.colCount(); ++i) {
                fila.push_back(row[i].isNull() ? "NULL" : row[i].get<std::string>());
            }
            resultado.filas.push_back(fila);
        }
        break;
    }
    default: {
        nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT(consulta));
        for (short i = 0; i < res.columns(); ++i) {
            resultado.columnas.push_back(res.column_name(i));
        }
        while (res.next()) {
            std::vector<std::string> fila;
            for (short j = 0; j < res.columns(); ++j) {
                fila.push_back(res.is_null(j) ? "NULL" : res.get<std::string>(j, "NULL"));
            }
            resultado.filas.push_back(fila);
        }
        break;
    }
    }
    return resultado;
}

void GestorAuditoria::crearFuncionesAuditoria() {
    if (motor_actual == MotorDB::MySQL) {
        ejecutarComando(env_plantillas.render_file("MySqlAuditFunctions.tpl", {}));
    }
    else if (motor_actual == MotorDB::SQLServer) {
        ejecutarComando(env_plantillas.render_file("SqlServerAuditFunctions.tpl", {}));
    }
}

void GestorAuditoria::generarAuditoriaParaTabla(const std::string& nombre_tabla) {
    switch (motor_actual) {
    case MotorDB::PostgreSQL: generarAuditoriaPostgreSQL(nombre_tabla); break;
    case MotorDB::SQLServer: generarAuditoriaSQLServer(nombre_tabla); break;
    case MotorDB::MySQL: generarAuditoriaMySQL(nombre_tabla); break;
    case MotorDB::SQLite: generarAuditoriaSQLite(nombre_tabla); break;
    }
}

void GestorAuditoria::generarAuditoriaPostgreSQL(const std::string& nombre_tabla) {
    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;
    ejecutarComando(env_plantillas.render_file("PostgresAudit.tpl", datos));
}

void GestorAuditoria::generarAuditoriaSQLServer(const std::string& nombre_tabla) {
    ejecutarComando("EXEC dbo.aud_trigger @tabla = '" + nombre_tabla + "'");
}

void GestorAuditoria::generarAuditoriaMySQL(const std::string& nombre_tabla) {
    conn_mysql->sql("CALL aud_trigger(?)").bind(nombre_tabla).execute();
    mysqlx::RowResult res = conn_mysql->sql("SELECT fcampos2(?, 'OLD')").bind(nombre_tabla).execute();

    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;
    datos["campos"] = res.fetchOne()[0].get<std::string>();

    ejecutarComando(env_plantillas.render_file("MySqlAuditTriggers.tpl", datos));
}

void GestorAuditoria::generarAuditoriaSQLite(const std::string& nombre_tabla) {
    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;

    std::string consulta_columnas = "PRAGMA table_info(" + nombre_tabla + ");";
    auto columnas_info = ejecutarConsultaConResultado(consulta_columnas);

    std::string definicion_columnas;
    for (size_t i = 0; i < columnas_info.size(); ++i) {
        definicion_columnas += columnas_info[i][1] + " " + columnas_info[i][2];
        if (i < columnas_info.size() - 1) {
            definicion_columnas += ", ";
        }
    }
    datos["definicion_columnas"] = definicion_columnas;

    std::string lista_columnas_old;
    for (size_t i = 0; i < columnas_info.size(); ++i) {
        lista_columnas_old += "OLD." + columnas_info[i][1];
        if (i < columnas_info.size() - 1) {
            lista_columnas_old += ", ";
        }
    }
    datos["lista_columnas_old"] = lista_columnas_old;

    ejecutarComando(env_plantillas.render_file("SqliteAudit.tpl", datos));
}