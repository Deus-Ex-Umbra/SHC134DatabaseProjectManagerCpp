#include "GestorAuditoria.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <vector>
#include "GestorCifrado.hpp"

GestorAuditoria::GestorAuditoria(MotorDB motor, const std::string& connection_string, const std::string& db)
    : motor_actual(motor), db_name(db) {
    conectar(connection_string, db);
}

GestorAuditoria::~GestorAuditoria() {
    desconectar();
}

void GestorAuditoria::setGestorCifrado(std::shared_ptr<GestorCifrado> gestor) {
    gestor_cifrado = gestor;
}

void GestorAuditoria::conectar(const std::string& connection_string, const std::string& db) {
    try {
        switch (motor_actual) {
        case MotorDB::PostgreSQL:
            conn_pg = PQconnectdb(connection_string.c_str());
            if (PQstatus(conn_pg) != CONNECTION_OK) throw std::runtime_error(PQerrorMessage(conn_pg));
            break;
        case MotorDB::MySQL:
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
    if (conn_odbc && conn_odbc->connected()) conn_odbc->disconnect();
}

bool GestorAuditoria::estaConectado() const {
    switch (motor_actual) {
    case MotorDB::PostgreSQL:
        return conn_pg && PQstatus(conn_pg) == CONNECTION_OK;
    default:
        return conn_odbc && conn_odbc->connected();
    }
}

GestorAuditoria::MotorDB GestorAuditoria::getMotor() const {
    return motor_actual;
}

void GestorAuditoria::ejecutarComando(const std::string& consulta) {
    try {
        if (motor_actual == MotorDB::PostgreSQL) {
            PGresult* res = PQexec(conn_pg, consulta.c_str());
            if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
                throw std::runtime_error(PQerrorMessage(conn_pg));
            }
            PQclear(res);
        }
        else {
            std::vector<std::string> statements;
            if (motor_actual == MotorDB::SQLServer) {
                boost::split(statements, consulta, boost::is_any_of("\n"), boost::token_compress_on);
                std::string current_batch;
                for (const auto& line : statements) {
                    if (boost::trim_copy(line) == "GO") {
                        if (!current_batch.empty()) {
                            nanodbc::just_execute(*conn_odbc, NANODBC_TEXT(current_batch));
                            current_batch.clear();
                        }
                    }
                    else {
                        current_batch += line + "\n";
                    }
                }
                if (!current_batch.empty()) {
                    nanodbc::just_execute(*conn_odbc, NANODBC_TEXT(current_batch));
                }
            }
            else if (motor_actual == MotorDB::MySQL) {
                std::string processed_query = consulta;
                boost::replace_all(processed_query, "DELIMITER $$", "---DELIMITER---");
                boost::replace_all(processed_query, "END$$", "---DELIMITER---");
                boost::split(statements, processed_query, boost::is_any_of("---DELIMITER---"), boost::token_compress_on);
                for (const auto& stmt : statements) {
                    if (!boost::trim_copy(stmt).empty()) {
                        nanodbc::just_execute(*conn_odbc, NANODBC_TEXT(stmt));
                    }
                }
            }
            else { // SQLite
                boost::split(statements, consulta, boost::is_any_of(";"), boost::token_compress_on);
                for (const auto& stmt : statements) {
                    if (!boost::trim_copy(stmt).empty()) {
                        nanodbc::just_execute(*conn_odbc, NANODBC_TEXT(stmt));
                    }
                }
            }
        }
    }
    catch (const nanodbc::database_error& e) {
        throw std::runtime_error("Error de Nanodbc: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error General: " + std::string(e.what()));
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
    crearFuncionesAuditoria();

    switch (motor_actual) {
    case MotorDB::PostgreSQL: generarAuditoriaPostgreSQL(nombre_tabla); break;
    case MotorDB::SQLServer: generarAuditoriaSQLServer(nombre_tabla); break;
    case MotorDB::MySQL: generarAuditoriaMySQL(nombre_tabla); break;
    case MotorDB::SQLite: generarAuditoriaSQLite(nombre_tabla); break;
    }
}

void GestorAuditoria::generarAuditoriaPostgreSQL(const std::string& nombre_tabla) {
    auto columnas_info_res = ejecutarConsultaConResultado("SELECT column_name FROM information_schema.columns WHERE table_name = '" + nombre_tabla + "' AND table_schema = 'public' ORDER BY ordinal_position;");

    std::ostringstream definicion_columnas;
    for (size_t i = 0; i < columnas_info_res.filas.size(); ++i) {
        definicion_columnas << "\"" << columnas_info_res.filas[i][0] << "\" TEXT";
        if (i < columnas_info_res.filas.size() - 1) {
            definicion_columnas << ", ";
        }
    }
    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;
    datos["definicion_columnas"] = definicion_columnas.str();
    ejecutarComando(env_plantillas.render_file("PostgresAudit.tpl", datos));
}

void GestorAuditoria::generarAuditoriaSQLServer(const std::string& nombre_tabla) {
    ejecutarComando("EXEC dbo.aud_trigger @tabla = N'" + nombre_tabla + "'");
}

void GestorAuditoria::generarAuditoriaMySQL(const std::string& nombre_tabla) {
    ejecutarComando("CALL aud_trigger('" + nombre_tabla + "')");

    auto res_new = ejecutarConsultaConResultado("SELECT fcampos2('" + nombre_tabla + "', 'NEW')");
    std::string campos_new = res_new.filas.front().front();

    auto res_old = ejecutarConsultaConResultado("SELECT fcampos2('" + nombre_tabla + "', 'OLD')");
    std::string campos_old = res_old.filas.front().front();

    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;
    datos["campos"] = campos_new;
    datos["campos_old"] = campos_old;

    ejecutarComando(env_plantillas.render_file("MySqlAuditTriggers.tpl", datos));
}

void GestorAuditoria::generarAuditoriaSQLite(const std::string& nombre_tabla) {
    auto columnas_info_res = ejecutarConsultaConResultado("PRAGMA table_info(" + nombre_tabla + ");");
    if (gestor_cifrado) {
        auto resultado = ejecutarConsultaConResultado("SELECT * FROM " + nombre_tabla);
        for (const auto& fila : resultado.filas) {
            gestor_cifrado->cifrarFilaEInsertar(nombre_tabla, columnas_info_res.columnas, fila, "Snapshot");
        }
    }
    else {
        nlohmann::json datos;
        datos["tabla"] = nombre_tabla;
        std::ostringstream definicion_columnas, lista_columnas_old, lista_columnas_new;

        for (size_t i = 0; i < columnas_info_res.filas.size(); ++i) {
            definicion_columnas << "\"" << columnas_info_res.filas[i][1] << "\" TEXT";
            lista_columnas_old << "OLD." << columnas_info_res.filas[i][1];
            lista_columnas_new << "NEW." << columnas_info_res.filas[i][1];
            if (i < columnas_info_res.filas.size() - 1) {
                definicion_columnas << ", ";
                lista_columnas_old << ", ";
                lista_columnas_new << ", ";
            }
        }

        datos["definicion_columnas"] = definicion_columnas.str();
        datos["lista_columnas_old"] = lista_columnas_old.str();
        datos["lista_columnas_new"] = lista_columnas_new.str();
        ejecutarComando(env_plantillas.render_file("SqliteAudit.tpl", datos));
    }
}