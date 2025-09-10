#include "GestorAuditoria.hpp"
#include <algorithm>
#include <iostream>
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
        crearFuncionesAuditoriaMySQL();
    }
    else if (motor_actual == MotorDB::SQLServer) {
        crearFuncionesAuditoriaSQLServer();
    }
}

void GestorAuditoria::crearFuncionesAuditoriaMySQL() {
    try {
        ejecutarComando("DROP FUNCTION IF EXISTS fcampos");
    }
    catch (...) {}

    std::string funcion_fcampos =
        "CREATE FUNCTION fcampos (tabla TEXT) RETURNS TEXT CHARSET utf8mb4 DETERMINISTIC "
        "BEGIN "
        "DECLARE valores TEXT DEFAULT ''; "
        "DECLARE nombre TEXT; "
        "DECLARE tipo TEXT; "
        "DECLARE finished INT DEFAULT 0; "
        "DECLARE cur CURSOR FOR SELECT column_name, column_type FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = tabla ORDER BY ordinal_position; "
        "DECLARE CONTINUE HANDLER FOR NOT FOUND SET finished = 1; "
        "OPEN cur; "
        "Bucle: LOOP "
        "FETCH cur INTO nombre, tipo; "
        "IF finished THEN LEAVE Bucle; END IF; "
        "SET valores = CONCAT(valores, nombre, ' ', tipo, ', '); "
        "END LOOP; "
        "CLOSE cur; "
        "SET valores = CONCAT(valores, 'UsuarioAccion TEXT, FechaAccion DATETIME, AccionSql TEXT'); "
        "RETURN valores; "
        "END";
    ejecutarComando(funcion_fcampos);

    try {
        ejecutarComando("DROP FUNCTION IF EXISTS fcampos2");
    }
    catch (...) {}

    std::string funcion_fcampos2 =
        "CREATE FUNCTION fcampos2 (tabla TEXT, prefijo TEXT) RETURNS TEXT CHARSET utf8mb4 DETERMINISTIC "
        "BEGIN "
        "DECLARE valores TEXT DEFAULT ''; "
        "DECLARE nombre TEXT; "
        "DECLARE finished INT DEFAULT 0; "
        "DECLARE cur CURSOR FOR SELECT column_name FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = tabla ORDER BY ordinal_position; "
        "DECLARE CONTINUE HANDLER FOR NOT FOUND SET finished = 1; "
        "OPEN cur; "
        "Bucle: LOOP "
        "FETCH cur INTO nombre; "
        "IF finished THEN LEAVE Bucle; END IF; "
        "SET valores = CONCAT(valores, prefijo, '.', nombre, ', '); "
        "END LOOP; "
        "CLOSE cur; "
        "SET valores = CONCAT(valores, 'SUBSTRING_INDEX(CURRENT_USER(),''@'',1), NOW()'); "
        "RETURN valores; "
        "END";
    ejecutarComando(funcion_fcampos2);

    try {
        ejecutarComando("DROP PROCEDURE IF EXISTS aud_trigger");
    }
    catch (...) {}

    std::string procedimiento_aud =
        "CREATE PROCEDURE aud_trigger(tabla TEXT) "
        "BEGIN "
        "DECLARE campos TEXT; "
        "SET campos = fcampos(tabla); "
        "SET @sql = CONCAT('DROP TABLE IF EXISTS aud_', tabla); "
        "PREPARE stmt FROM @sql; "
        "EXECUTE stmt; "
        "DEALLOCATE PREPARE stmt; "
        "SET @sql = CONCAT('CREATE TABLE aud_', tabla, ' (', campos, ')'); "
        "PREPARE stmt FROM @sql; "
        "EXECUTE stmt; "
        "DEALLOCATE PREPARE stmt; "
        "END";
    ejecutarComando(procedimiento_aud);
}

void GestorAuditoria::crearFuncionesAuditoriaSQLServer() {
    try {
        ejecutarComando("IF OBJECT_ID('fcampos', 'FN') IS NOT NULL DROP FUNCTION fcampos");
    }
    catch (...) {}

    std::string funcion_fcampos =
        "CREATE FUNCTION fcampos (@tabla VARCHAR(250)) RETURNS VARCHAR(MAX) AS "
        "BEGIN "
        "DECLARE @valores varchar(MAX) = ''; "
        "DECLARE @Nombre varchar(250), @tipo VARCHAR(50), @longitud VARCHAR(50); "
        "DECLARE cur CURSOR FOR SELECT C.name, T.name, C.max_length FROM sys.columns C INNER JOIN sys.types T ON T.system_type_id=C.system_type_id WHERE C.object_id = OBJECT_ID(@tabla) ORDER BY C.column_id; "
        "OPEN cur; "
        "FETCH NEXT FROM cur INTO @Nombre,@tipo,@longitud; "
        "WHILE (@@FETCH_STATUS = 0) BEGIN "
        "IF @tipo IN ('varchar','char','numeric','nvarchar','nchar') "
        "IF @longitud = '-1' SET @valores = @valores + @Nombre + ' '+ @tipo + '(MAX), '; "
        "ELSE SET @valores = @valores + @Nombre + ' '+ @tipo + '('+ @longitud + '), '; "
        "ELSE SET @valores = @valores + @Nombre + ' '+ @tipo + ', '; "
        "FETCH NEXT FROM cur INTO @Nombre,@tipo,@longitud; "
        "END; "
        "CLOSE cur; "
        "DEALLOCATE cur; "
        "SET @valores = @valores + 'UsuarioAccion varchar(30), FechaAccion datetime, AccionSql varchar(30)'; "
        "RETURN @valores; "
        "END";
    ejecutarComando(funcion_fcampos);

    try {
        ejecutarComando("IF OBJECT_ID('aud_trigger', 'P') IS NOT NULL DROP PROCEDURE aud_trigger");
    }
    catch (...) {}

    std::string procedimiento_aud =
        "CREATE PROC aud_trigger @tabla VARCHAR(250) AS "
        "BEGIN "
        "DECLARE @campos varchar(MAX) = dbo.fcampos(@tabla), @createTable VARCHAR(MAX), @trigger VARCHAR(MAX); "
        "SET @createTable = 'CREATE TABLE Aud'+@tabla+' (' +@campos+ ')'; "
        "SET @trigger = 'CREATE TRIGGER Trg'+@tabla+'Aud ON dbo.'+@tabla+' AFTER INSERT, UPDATE, DELETE AS BEGIN "
        "IF EXISTS(SELECT * FROM INSERTED) AND NOT EXISTS(SELECT * FROM DELETED) BEGIN "
        "INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Insertado'' FROM INSERTED; END "
        "ELSE IF EXISTS(SELECT * FROM INSERTED) AND EXISTS(SELECT * FROM DELETED) BEGIN "
        "INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Modificado'' FROM DELETED; END "
        "ELSE IF EXISTS(SELECT * FROM DELETED) BEGIN "
        "INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Eliminado'' FROM DELETED; END; END;'; "
        "IF OBJECT_ID('Aud'+@tabla, 'U') IS NOT NULL EXEC('DROP TABLE Aud'+@tabla); "
        "IF OBJECT_ID('Trg'+@tabla+'Aud', 'TR') IS NOT NULL EXEC('DROP TRIGGER Trg'+@tabla+'Aud'); "
        "EXEC(@createTable); "
        "EXEC(@trigger); "
        "END";
    ejecutarComando(procedimiento_aud);
}

void GestorAuditoria::generarAuditoriaParaTabla(const std::string& nombre_tabla) {
    if (estaConectado()) {
        crearFuncionesAuditoria();
    }

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
    ejecutarComando("CALL aud_trigger('" + nombre_tabla + "')");

    nanodbc::result res = nanodbc::execute(*conn_odbc, NANODBC_TEXT("SELECT fcampos2('" + nombre_tabla + "', 'NEW')"));
    std::string campos_new;
    if (res.next()) campos_new = res.get<std::string>(0);

    nanodbc::result res2 = nanodbc::execute(*conn_odbc, NANODBC_TEXT("SELECT fcampos2('" + nombre_tabla + "', 'OLD')"));
    std::string campos_old;
    if (res2.next()) campos_old = res2.get<std::string>(0);

    nlohmann::json datos;
    datos["tabla"] = nombre_tabla;
    datos["campos"] = campos_new;
    datos["campos_old"] = campos_old;

    std::string triggers = env_plantillas.render_file("MySqlAuditTriggers.tpl", datos);

    std::istringstream stream(triggers);
    std::string linea;
    std::string trigger_actual;
    while (std::getline(stream, linea)) {
        if (linea.find("DROP TRIGGER") != std::string::npos ||
            (linea.find("CREATE TRIGGER") != std::string::npos && !trigger_actual.empty())) {
            if (!trigger_actual.empty()) {
                ejecutarComando(trigger_actual);
                trigger_actual.clear();
            }
        }
        trigger_actual += linea + " ";
    }
    if (!trigger_actual.empty()) {
        ejecutarComando(trigger_actual);
    }
}

void GestorAuditoria::generarAuditoriaSQLite(const std::string& nombre_tabla) {
    auto columnas_info_res = ejecutarConsultaConResultado("PRAGMA table_info(" + nombre_tabla + ");");
    if (gestor_cifrado) {
        std::cout << "Generando auditoria cifrada para SQLite (a nivel de aplicacion)..." << std::endl;
        auto resultado = ejecutarConsultaConResultado("SELECT * FROM " + nombre_tabla);
        for (const auto& fila : resultado.filas) {
            gestor_cifrado->cifrarFilaEInsertar(nombre_tabla, columnas_info_res.columnas, fila, "Snapshot");
        }
    }
    else {
        nlohmann::json datos;
        datos["tabla"] = nombre_tabla;
        std::string definicion_columnas;
        for (size_t i = 0; i < columnas_info_res.filas.size(); ++i) {
            definicion_columnas += columnas_info_res.filas[i][1] + " " + columnas_info_res.filas[i][2];
            if (i < columnas_info_res.filas.size() - 1) {
                definicion_columnas += ", ";
            }
        }
        datos["definicion_columnas"] = definicion_columnas;

        std::string lista_columnas_old;
        std::string lista_columnas_new;
        for (size_t i = 0; i < columnas_info_res.filas.size(); ++i) {
            lista_columnas_old += "OLD." + columnas_info_res.filas[i][1];
            lista_columnas_new += "NEW." + columnas_info_res.filas[i][1];
            if (i < columnas_info_res.filas.size() - 1) {
                lista_columnas_old += ", ";
                lista_columnas_new += ", ";
            }
        }
        datos["lista_columnas_old"] = lista_columnas_old;
        datos["lista_columnas_new"] = lista_columnas_new;
        ejecutarComando(env_plantillas.render_file("SqliteAudit.tpl", datos));
    }
}