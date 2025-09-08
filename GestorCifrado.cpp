#include "GestorCifrado.hpp"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

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

GestorCifrado::GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion_hex)
    : gestor_db(gestor) {
    if (clave_encriptacion_hex.length() != 64) {
        throw std::runtime_error("La clave de encriptacion debe ser una cadena hexadecimal de 64 caracteres.");
    }
    clave = hexABytes(clave_encriptacion_hex);
}

void GestorCifrado::cifrarFilaEInsertar(const std::string& tabla, const std::vector<std::string>& columnas, const std::vector<std::string>& fila, const std::string& accion) {
    std::string tabla_auditoria = "aud_" + tabla;
    std::string insert_sql = "INSERT INTO " + tabla_auditoria + " (";
    std::string values_sql = ") VALUES (";
    bool first = true;

    for (const auto& col : columnas) {
        if (!first) {
            insert_sql += ", ";
            values_sql += ", ";
        }
        insert_sql += "\"" + cifrarValor(col) + "\"";
        first = false;
    }
    insert_sql += ", UsuarioAccion, FechaAccion, AccionSql";

    first = true;
    for (const auto& val : fila) {
        if (!first) values_sql += ", ";
        values_sql += "'" + cifrarValor(val) + "'";
        first = false;
    }
    values_sql += ", 'SYSTEM', datetime('now'), '" + accion + "')";

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
    gestor_db->ejecutarComando("IF NOT EXISTS (SELECT * FROM sys.symmetric_keys WHERE name = 'AuditoriaKey') BEGIN CREATE MASTER KEY ENCRYPTION BY PASSWORD = 'DevPasswordComplexEnough#123'; CREATE CERTIFICATE AuditoriaCert WITH SUBJECT = 'Certificado para Cifrado de Auditoria'; CREATE SYMMETRIC KEY AuditoriaKey WITH ALGORITHM = AES_256 ENCRYPTION BY CERTIFICATE AuditoriaCert; END;");
}

void GestorCifrado::actualizarTriggersParaCifrado(const std::string& nombre_tabla, const std::vector<std::string>& columnas_originales) {
    std::cout << "Actualizando triggers para la tabla cifrada: " << nombre_tabla << std::endl;

    nlohmann::json datos;
    std::string tabla_original = nombre_tabla.substr(nombre_tabla.rfind("aud_") == 0 ? 4 : (nombre_tabla.rfind("Aud") == 0 ? 3 : 0));
    datos["tabla"] = tabla_original;
    datos["tabla_auditoria"] = nombre_tabla;
    datos["clave_hex"] = bytesAHex(clave.data(), clave.size());

    std::vector<nlohmann::json> columnas_json;
    for (const auto& col_name : columnas_originales) {
        if (col_name != "UsuarioAccion" && col_name != "FechaAccion" && col_name != "AccionSql") {
            columnas_json.push_back({
                {"nombre", col_name},
                {"nombre_cifrado", cifrarValor(col_name)}
                });
        }
    }
    datos["columnas"] = columnas_json;

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
    default:
        std::cout << "Advertencia: La actualizacion de triggers para cifrado no esta implementada para este motor de base de datos." << std::endl;
        return;
    }
    gestor_db->ejecutarComando(sql_comando);
}

void GestorCifrado::cifrarTablasDeAuditoria() {
    std::vector<std::string> todas_las_tablas = gestor_db->obtenerNombresDeTablas(true);
    std::vector<std::string> tablas_auditoria;
    std::copy_if(todas_las_tablas.begin(), todas_las_tablas.end(), std::back_inserter(tablas_auditoria),
        [](const std::string& s) { return s.rfind("aud_", 0) == 0 || s.rfind("Aud", 0) == 0; });

    if (tablas_auditoria.empty()) {
        std::cout << "No se encontraron tablas de auditoria para cifrar." << std::endl;
        return;
    }

    for (const auto& tabla : tablas_auditoria) {
        std::cout << "Cifrando tabla: " << tabla << std::endl;
        auto resultado = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla);

        if (!resultado.filas.empty()) {
            for (const auto& fila : resultado.filas) {
                std::string update_sql = "UPDATE " + tabla + " SET ";
                std::string where_clause = " WHERE ";
                bool first_set = true;
                bool first_where = true;

                for (size_t i = 0; i < resultado.columnas.size(); ++i) {
                    if (resultado.columnas[i] != "UsuarioAccion" && resultado.columnas[i] != "FechaAccion" && resultado.columnas[i] != "AccionSql") {
                        if (!first_set) update_sql += ", ";
                        update_sql += "\"" + resultado.columnas[i] + "\" = '" + cifrarValor(fila[i]) + "'";
                        first_set = false;
                    }
                    if (!first_where) where_clause += " AND ";
                    where_clause += "\"" + resultado.columnas[i] + "\" IS NOT DISTINCT FROM '" + fila[i] + "'";
                    first_where = false;
                }
                if (!first_set) gestor_db->ejecutarComando(update_sql + where_clause);
            }
        }

        std::vector<std::string> columnas_originales;
        for (const auto& col : resultado.columnas) {
            columnas_originales.push_back(col);
            if (col != "UsuarioAccion" && col != "FechaAccion" && col != "AccionSql") {
                std::string cifrado_col = cifrarValor(col);
                std::string rename_sql = "ALTER TABLE " + tabla + " RENAME COLUMN \"" + col + "\" TO \"" + cifrado_col + "\"";
                gestor_db->ejecutarComando(rename_sql);
            }
        }

        actualizarTriggersParaCifrado(tabla, columnas_originales);
    }
    std::cout << "Cifrado de tablas de auditoria completado." << std::endl;
}

std::vector<std::vector<std::string>> GestorCifrado::ejecutarConsultaConDesencriptado(const std::string& consulta) {
    auto resultado_cifrado = gestor_db->ejecutarConsultaConResultado(consulta);
    std::vector<std::vector<std::string>> resultado_final;

    std::vector<std::string> cabeceras_descifradas;
    for (const auto& col : resultado_cifrado.columnas) {
        cabeceras_descifradas.push_back(descifrarValor(col));
    }
    resultado_final.push_back(cabeceras_descifradas);

    for (const auto& fila : resultado_cifrado.filas) {
        std::vector<std::string> fila_descifrada;
        for (const auto& celda : fila) {
            fila_descifrada.push_back(descifrarValor(celda));
        }
        resultado_final.push_back(fila_descifrada);
    }
    return resultado_final;
}