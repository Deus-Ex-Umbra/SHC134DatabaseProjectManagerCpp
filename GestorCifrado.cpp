#include "GestorCifrado.hpp"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <map>

std::vector<unsigned char> hexABytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    bytes.reserve(hex.length() / 2);
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytesAHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    return ss.str();
}

std::string cifrarNombreColumnaCesar(const std::string& texto_plano, int desplazamiento) {
    // Conjunto ampliado de caracteres permitidos en nombres de columnas de BD
    const std::string caracteres_permitidos =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_"; // Solo underscore es universalmente permitido en nombres de columnas

    std::string resultado;

    for (char c : texto_plano) {
        size_t pos = caracteres_permitidos.find(c);
        if (pos != std::string::npos) {
            size_t nueva_pos = (pos + desplazamiento) % caracteres_permitidos.length();
            resultado += caracteres_permitidos[nueva_pos];
        }
        else {
            // Si el caracter no está en el conjunto, lo dejamos igual
            resultado += c;
        }
    }
    return resultado;
}

std::string descifrarNombreColumnaCesar(const std::string& texto_cifrado, int desplazamiento) {
    const std::string caracteres_permitidos =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_";

    std::string resultado;

    for (char c : texto_cifrado) {
        size_t pos = caracteres_permitidos.find(c);
        if (pos != std::string::npos) {
            int nueva_pos = (pos - desplazamiento + caracteres_permitidos.length()) % caracteres_permitidos.length();
            resultado += caracteres_permitidos[nueva_pos];
        }
        else {
            resultado += c;
        }
    }
    return resultado;
}

GestorCifrado::GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion_hex)
    : gestor_db(gestor), clave_hex(clave_encriptacion_hex) {
    if (clave_encriptacion_hex.length() != 64) {
        throw std::runtime_error("La clave de encriptacion debe ser una cadena hexadecimal de 64 caracteres.");
    }
    clave = hexABytes(clave_encriptacion_hex);

    // Calcular desplazamiento basado en el primer dígito hexadecimal
    char primer_digito = clave_encriptacion_hex[0];
    if (primer_digito >= '0' && primer_digito <= '9') {
        desplazamiento_cesar = primer_digito - '0';
    }
    else if (primer_digito >= 'a' && primer_digito <= 'f') {
        desplazamiento_cesar = primer_digito - 'a' + 10;
    }
    else if (primer_digito >= 'A' && primer_digito <= 'F') {
        desplazamiento_cesar = primer_digito - 'A' + 10;
    }

    std::cout << "Desplazamiento Cesar calculado: " << desplazamiento_cesar << std::endl;
}

void GestorCifrado::cifrarFilaEInsertar(const std::string& tabla, const std::vector<std::string>& columnas, const std::vector<std::string>& fila, const std::string& accion) {
    std::string tabla_auditoria = "aud_" + tabla;
    std::string insert_sql = "INSERT INTO " + tabla_auditoria + " (";
    std::string values_sql = ") VALUES (";
    bool first = true;

    for (size_t i = 0; i < columnas.size(); ++i) {
        if (!first) {
            insert_sql += ", ";
            values_sql += ", ";
        }
        insert_sql += "\"" + cifrarNombreColumnaCesar(columnas[i], desplazamiento_cesar) + "\"";
        values_sql += "'" + cifrarValor(fila[i]) + "'";
        first = false;
    }

    std::string usuario_cifrado = cifrarNombreColumnaCesar("UsuarioAccion", desplazamiento_cesar);
    std::string fecha_cifrada = cifrarNombreColumnaCesar("FechaAccion", desplazamiento_cesar);
    std::string accion_cifrada = cifrarNombreColumnaCesar("AccionSql", desplazamiento_cesar);

    insert_sql += ", \"" + usuario_cifrado + "\", \"" + fecha_cifrada + "\", \"" + accion_cifrada + "\"";

    std::string fecha_actual = "";
    if (gestor_db->getMotor() == GestorAuditoria::MotorDB::SQLite) {
        auto resultado = gestor_db->ejecutarConsultaConResultado("SELECT datetime('now')");
        if (!resultado.filas.empty() && !resultado.filas[0].empty()) {
            fecha_actual = resultado.filas[0][0];
        }
    }

    values_sql += ", '" + cifrarValor("SYSTEM") + "', '" + cifrarValor(fecha_actual.empty() ? "2024-01-01 00:00:00" : fecha_actual) + "', '" + cifrarValor(accion) + "')";

    gestor_db->ejecutarComando(insert_sql + values_sql);
}

std::string GestorCifrado::cifrarValor(const std::string& texto_plano) {
    if (texto_plano.empty() || texto_plano == "NULL") {
        return texto_plano;
    }

    unsigned char iv[AES_BLOCK_SIZE];
    if (!RAND_bytes(iv, sizeof(iv))) {
        throw std::runtime_error("Error al generar el IV aleatorio.");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de cifrado.");

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), iv);

    std::vector<unsigned char> texto_cifrado(texto_plano.length() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, texto_cifrado.data(), &len, (const unsigned char*)texto_plano.c_str(), texto_plano.length());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, texto_cifrado.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return bytesAHex(iv, AES_BLOCK_SIZE) + bytesAHex(texto_cifrado.data(), ciphertext_len);
}

std::string GestorCifrado::descifrarValor(const std::string& texto_cifrado_hex) {
    if (texto_cifrado_hex.length() < 32 || texto_cifrado_hex.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        return texto_cifrado_hex;
    }

    try {
        std::vector<unsigned char> iv = hexABytes(texto_cifrado_hex.substr(0, 32));
        std::vector<unsigned char> texto_cifrado = hexABytes(texto_cifrado_hex.substr(32));

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "[ERROR_CTX]";

        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), iv.data())) {
            EVP_CIPHER_CTX_free(ctx);
            return "[ERROR_INIT]";
        }

        std::vector<unsigned char> texto_plano(texto_cifrado.size() + AES_BLOCK_SIZE);
        int len = 0;
        int plaintext_len = 0;

        if (1 != EVP_DecryptUpdate(ctx, texto_plano.data(), &len, texto_cifrado.data(), texto_cifrado.size())) {
            EVP_CIPHER_CTX_free(ctx);
            return "[ERROR_UPDATE]";
        }
        plaintext_len = len;

        if (1 != EVP_DecryptFinal_ex(ctx, texto_plano.data() + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            return "[ERROR_FINAL]";
        }
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        return std::string((char*)texto_plano.data(), plaintext_len);
    }
    catch (const std::exception&) {
        return texto_cifrado_hex;
    }
}

void GestorCifrado::prepararCifradoSQLServer() {
    std::cout << "Preparando la base de datos SQL Server para el cifrado..." << std::endl;

    try {
        // Verificar si ya existe la clave maestra
        auto resultado = gestor_db->ejecutarConsultaConResultado("SELECT * FROM sys.symmetric_keys WHERE name = 'AuditoriaKey'");
        if (resultado.filas.empty()) {
            gestor_db->ejecutarComando("CREATE MASTER KEY ENCRYPTION BY PASSWORD = 'DevPasswordComplexEnough#123'");
            gestor_db->ejecutarComando("CREATE CERTIFICATE AuditoriaCert WITH SUBJECT = 'Certificado para Cifrado de Auditoria'");
            gestor_db->ejecutarComando("CREATE SYMMETRIC KEY AuditoriaKey WITH ALGORITHM = AES_256 ENCRYPTION BY CERTIFICATE AuditoriaCert");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Advertencia en preparacion SQL Server: " << e.what() << std::endl;
    }
}

void GestorCifrado::actualizarTriggersParaCifrado(const std::string& nombre_tabla, const std::map<std::string, std::string>& mapa_columnas) {
    std::cout << "Actualizando triggers para la tabla cifrada: " << nombre_tabla << std::endl;

    nlohmann::json datos;
    std::string tabla_original = nombre_tabla;

    // Obtener nombre de tabla original (sin prefijo aud_)
    if (boost::starts_with(tabla_original, "aud_")) {
        tabla_original = tabla_original.substr(4);
    }
    else if (boost::starts_with(tabla_original, "Aud")) {
        tabla_original = tabla_original.substr(3);
    }

    datos["tabla"] = tabla_original;
    datos["tabla_auditoria"] = nombre_tabla;

    // Construir listas de columnas y valores cifrados
    std::ostringstream columnas_cifradas_lista;
    std::ostringstream valores_insert_new;
    std::ostringstream valores_update_old;
    std::ostringstream valores_delete_old;
    bool first_col = true;

    for (const auto& par : mapa_columnas) {
        const std::string& col_original = par.first;
        const std::string& col_cifrada = par.second;

        if (!first_col) {
            columnas_cifradas_lista << ", ";
            valores_insert_new << ", ";
            valores_update_old << ", ";
            valores_delete_old << ", ";
        }

        columnas_cifradas_lista << "\"" << col_cifrada << "\"";

        switch (gestor_db->getMotor()) {
        case GestorAuditoria::MotorDB::PostgreSQL:
            valores_insert_new << "encode(encrypt(COALESCE(NEW.\"" << col_original << "\"::TEXT, 'NULL'), '"
                << clave_hex << "', 'aes'), 'hex')";
            valores_update_old << "encode(encrypt(COALESCE(OLD.\"" << col_original << "\"::TEXT, 'NULL'), '"
                << clave_hex << "', 'aes'), 'hex')";
            valores_delete_old << "encode(encrypt(COALESCE(OLD.\"" << col_original << "\"::TEXT, 'NULL'), '"
                << clave_hex << "', 'aes'), 'hex')";
            break;

        case GestorAuditoria::MotorDB::MySQL:
            valores_insert_new << "HEX(AES_ENCRYPT(COALESCE(NEW." << col_original << ", 'NULL'), '"
                << clave_hex.substr(0, 32) << "'))";
            valores_update_old << "HEX(AES_ENCRYPT(COALESCE(OLD." << col_original << ", 'NULL'), '"
                << clave_hex.substr(0, 32) << "'))";
            valores_delete_old << "HEX(AES_ENCRYPT(COALESCE(OLD." << col_original << ", 'NULL'), '"
                << clave_hex.substr(0, 32) << "'))";
            break;

        case GestorAuditoria::MotorDB::SQLServer:
            valores_insert_new << "CONVERT(VARCHAR(MAX), ENCRYPTBYKEY(KEY_GUID('AuditoriaKey'), "
                << "CAST(COALESCE(i." << col_original << ", 'NULL') AS VARCHAR(MAX))), 2)";
            valores_update_old << "CONVERT(VARCHAR(MAX), ENCRYPTBYKEY(KEY_GUID('AuditoriaKey'), "
                << "CAST(COALESCE(d." << col_original << ", 'NULL') AS VARCHAR(MAX))), 2)";
            valores_delete_old << "CONVERT(VARCHAR(MAX), ENCRYPTBYKEY(KEY_GUID('AuditoriaKey'), "
                << "CAST(COALESCE(d." << col_original << ", 'NULL') AS VARCHAR(MAX))), 2)";
            break;

        case GestorAuditoria::MotorDB::SQLite:
            // SQLite no soporta cifrado nativo, usar valor sin cifrar temporalmente
            valores_insert_new << "NEW." << col_original;
            valores_update_old << "OLD." << col_original;
            valores_delete_old << "OLD." << col_original;
            break;
        }

        first_col = false;
    }

    datos["lista_columnas_cifradas"] = columnas_cifradas_lista.str();
    datos["valores_insert_new"] = valores_insert_new.str();
    datos["valores_update_old"] = valores_update_old.str();
    datos["valores_delete_old"] = valores_delete_old.str();

    // Generar y ejecutar triggers según el motor
    std::string sql_comando;
    switch (gestor_db->getMotor()) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        sql_comando = env_plantillas.render_file("PostgresAuditCifrado.tpl", datos);
        break;
    case GestorAuditoria::MotorDB::MySQL:
        sql_comando = env_plantillas.render_file("MySqlAuditCifrado.tpl", datos);
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        prepararCifradoSQLServer();
        sql_comando = env_plantillas.render_file("SqlServerAuditCifrado.tpl", datos);
        break;
    case GestorAuditoria::MotorDB::SQLite:
        std::cout << "Advertencia: SQLite requiere cifrado a nivel de aplicacion." << std::endl;
        return;
    }

    // Ejecutar el SQL generado
    if (!sql_comando.empty()) {
        std::istringstream stream(sql_comando);
        std::string linea;
        std::string comando_actual;

        while (std::getline(stream, linea)) {
            comando_actual += linea + " ";

            // Ejecutar cuando encontramos un delimitador de comando
            if (linea.find(";") != std::string::npos ||
                linea.find("GO") != std::string::npos) {
                try {
                    gestor_db->ejecutarComando(comando_actual);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error ejecutando trigger: " << e.what() << std::endl;
                }
                comando_actual.clear();
            }
        }
    }
}

bool GestorCifrado::esColumnaCifrada(const std::string& nombre_columna) {
    if (nombre_columna.empty()) return false;

    // Las columnas de auditoría nunca se cifran
    if (boost::iequals(nombre_columna, "UsuarioAccion") ||
        boost::iequals(nombre_columna, "FechaAccion") ||
        boost::iequals(nombre_columna, "AccionSql") ||
        boost::iequals(nombre_columna, "__temp_id")) {
        return false;
    }

    // Verificar si el nombre está cifrado con César
    std::string posible_original = descifrarNombreColumnaCesar(nombre_columna, desplazamiento_cesar);
    return posible_original != nombre_columna;
}

void GestorCifrado::cifrarTablasDeAuditoria() {
    std::cout << "Iniciando cifrado de tablas de auditoria..." << std::endl;

    // Obtener todas las tablas que empiezan con aud_
    auto tablas_auditoria = gestor_db->obtenerNombresDeTablas(true);
    tablas_auditoria.erase(
        std::remove_if(tablas_auditoria.begin(), tablas_auditoria.end(),
            [](const std::string& s) {
                return !boost::starts_with(s, "aud_") && !boost::starts_with(s, "Aud");
            }),
        tablas_auditoria.end()
    );

    if (tablas_auditoria.empty()) {
        std::cout << "No se encontraron tablas de auditoria para cifrar." << std::endl;
        return;
    }

    for (const auto& tabla : tablas_auditoria) {
        std::cout << "\nProcesando tabla de auditoria: " << tabla << std::endl;

        try {
            // Paso 1: Obtener estructura de la tabla
            auto resultado_columnas = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla + " LIMIT 0");
            std::vector<std::string> columnas_a_cifrar;
            std::map<std::string, std::string> mapa_columnas_cifradas;

            // Identificar columnas a cifrar (todas excepto las de auditoría)
            for (const auto& col : resultado_columnas.columnas) {
                if (col != "UsuarioAccion" && col != "FechaAccion" && col != "AccionSql") {
                    columnas_a_cifrar.push_back(col);
                    std::string nombre_cifrado = cifrarNombreColumnaCesar(col, desplazamiento_cesar);
                    mapa_columnas_cifradas[col] = nombre_cifrado;
                    std::cout << "  Columna a cifrar: " << col << " -> " << nombre_cifrado << std::endl;
                }
            }

            if (columnas_a_cifrar.empty()) {
                std::cout << "  No hay columnas para cifrar en esta tabla." << std::endl;
                continue;
            }

            // Paso 2: Cambiar todas las columnas a TEXT
            for (const auto& col : columnas_a_cifrar) {
                std::string alter_sql;
                switch (gestor_db->getMotor()) {
                case GestorAuditoria::MotorDB::PostgreSQL:
                    alter_sql = "ALTER TABLE " + tabla + " ALTER COLUMN \"" + col + "\" TYPE TEXT";
                    break;
                case GestorAuditoria::MotorDB::MySQL:
                    alter_sql = "ALTER TABLE " + tabla + " MODIFY COLUMN `" + col + "` TEXT";
                    break;
                case GestorAuditoria::MotorDB::SQLServer:
                    alter_sql = "ALTER TABLE " + tabla + " ALTER COLUMN [" + col + "] VARCHAR(MAX)";
                    break;
                case GestorAuditoria::MotorDB::SQLite:
                    // SQLite no permite ALTER COLUMN directamente
                    break;
                }

                if (!alter_sql.empty()) {
                    try {
                        gestor_db->ejecutarComando(alter_sql);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "  Advertencia al cambiar tipo de columna: " << e.what() << std::endl;
                    }
                }
            }

            // Paso 3: Agregar columna temporal para identificar filas
            std::string add_temp_id;
            switch (gestor_db->getMotor()) {
            case GestorAuditoria::MotorDB::PostgreSQL:
                add_temp_id = "ALTER TABLE " + tabla + " ADD COLUMN __temp_id SERIAL";
                break;
            case GestorAuditoria::MotorDB::MySQL:
                add_temp_id = "ALTER TABLE " + tabla + " ADD COLUMN __temp_id INT AUTO_INCREMENT PRIMARY KEY";
                break;
            case GestorAuditoria::MotorDB::SQLServer:
                add_temp_id = "ALTER TABLE " + tabla + " ADD __temp_id INT IDENTITY(1,1)";
                break;
            case GestorAuditoria::MotorDB::SQLite:
                add_temp_id = "ALTER TABLE " + tabla + " ADD COLUMN __temp_id INTEGER";
                break;
            }

            try {
                gestor_db->ejecutarComando(add_temp_id);
            }
            catch (const std::exception& e) {
                std::cerr << "  Advertencia al agregar columna temporal: " << e.what() << std::endl;
            }

            // Paso 4: Obtener todos los datos y cifrarlos
            auto datos_con_id = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla);
            int filas_cifradas = 0;

            for (const auto& fila : datos_con_id.filas) {
                std::string update_sql = "UPDATE " + tabla + " SET ";
                std::string where_clause;
                bool first_set = true;

                for (size_t i = 0; i < datos_con_id.columnas.size(); ++i) {
                    const auto& col_name = datos_con_id.columnas[i];

                    if (col_name == "__temp_id") {
                        where_clause = " WHERE __temp_id = " + fila[i];
                        continue;
                    }

                    // Cifrar solo las columnas que no son de auditoría
                    if (std::find(columnas_a_cifrar.begin(), columnas_a_cifrar.end(), col_name) != columnas_a_cifrar.end()) {
                        if (!first_set) update_sql += ", ";

                        std::string valor_original = fila[i];
                        std::string valor_cifrado = cifrarValor(valor_original);

                        // Escapar comillas simples
                        boost::replace_all(valor_cifrado, "'", "''");

                        update_sql += "\"" + col_name + "\" = '" + valor_cifrado + "'";
                        first_set = false;
                    }
                }

                if (!first_set && !where_clause.empty()) {
                    try {
                        gestor_db->ejecutarComando(update_sql + where_clause);
                        filas_cifradas++;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "  Error cifrando fila: " << e.what() << std::endl;
                    }
                }
            }

            std::cout << "  Filas cifradas: " << filas_cifradas << std::endl;

            // Paso 5: Eliminar columna temporal
            std::string drop_temp_id = "ALTER TABLE " + tabla + " DROP COLUMN __temp_id";
            try {
                gestor_db->ejecutarComando(drop_temp_id);
            }
            catch (const std::exception& e) {
                std::cerr << "  Advertencia al eliminar columna temporal: " << e.what() << std::endl;
            }

            // Paso 6: Renombrar columnas con nombres cifrados
            for (const auto& par : mapa_columnas_cifradas) {
                std::string rename_sql;
                switch (gestor_db->getMotor()) {
                case GestorAuditoria::MotorDB::PostgreSQL:
                    rename_sql = "ALTER TABLE " + tabla + " RENAME COLUMN \"" + par.first + "\" TO \"" + par.second + "\"";
                    break;
                case GestorAuditoria::MotorDB::MySQL:
                    rename_sql = "ALTER TABLE " + tabla + " CHANGE COLUMN `" + par.first + "` `" + par.second + "` TEXT";
                    break;
                case GestorAuditoria::MotorDB::SQLServer:
                    rename_sql = "EXEC sp_rename '" + tabla + "." + par.first + "', '" + par.second + "', 'COLUMN'";
                    break;
                case GestorAuditoria::MotorDB::SQLite:
                    // SQLite no soporta RENAME COLUMN en versiones antiguas
                    break;
                }

                if (!rename_sql.empty()) {
                    try {
                        gestor_db->ejecutarComando(rename_sql);
                        std::cout << "  Columna renombrada: " << par.first << " -> " << par.second << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "  Error renombrando columna: " << e.what() << std::endl;
                    }
                }
            }

            // Paso 7: Actualizar triggers para cifrado automático
            if (!mapa_columnas_cifradas.empty()) {
                actualizarTriggersParaCifrado(tabla, mapa_columnas_cifradas);
            }

            std::cout << "  Tabla " << tabla << " cifrada exitosamente." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error procesando tabla " << tabla << ": " << e.what() << std::endl;
        }
    }

    std::cout << "\nCifrado de tablas de auditoria completado." << std::endl;
}

std::vector<std::vector<std::string>> GestorCifrado::ejecutarConsultaConDesencriptado(const std::string& consulta) {
    auto resultado_cifrado = gestor_db->ejecutarConsultaConResultado(consulta);
    std::vector<std::vector<std::string>> resultado_final;

    // Descifrar nombres de columnas
    std::vector<std::string> cabeceras_descifradas;
    for (const auto& col : resultado_cifrado.columnas) {
        if (esColumnaCifrada(col)) {
            cabeceras_descifradas.push_back(descifrarNombreColumnaCesar(col, desplazamiento_cesar));
        }
        else {
            cabeceras_descifradas.push_back(col);
        }
    }
    resultado_final.push_back(cabeceras_descifradas);

    // Descifrar datos
    for (const auto& fila : resultado_cifrado.filas) {
        std::vector<std::string> fila_descifrada;
        for (size_t i = 0; i < fila.size(); ++i) {
            if (esColumnaCifrada(resultado_cifrado.columnas[i])) {
                fila_descifrada.push_back(descifrarValor(fila[i]));
            }
            else {
                fila_descifrada.push_back(fila[i]);
            }
        }
        resultado_final.push_back(fila_descifrada);
    }

    return resultado_final;
}

std::string GestorCifrado::getClave() const {
    return clave_hex;
}