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
    const std::string caracteres_permitidos = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    std::string resultado;

    for (char c : texto_plano) {
        size_t pos = caracteres_permitidos.find(c);
        if (pos != std::string::npos) {
            size_t nueva_pos = (pos + desplazamiento) % caracteres_permitidos.length();
            resultado += caracteres_permitidos[nueva_pos];
        }
        else {
            resultado += c;
        }
    }
    return resultado;
}

std::string descifrarNombreColumnaCesar(const std::string& texto_cifrado, int desplazamiento) {
    const std::string caracteres_permitidos = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
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
        insert_sql += "\"" + cifrarNombreColumnaCesar(col, desplazamiento_cesar) + "\"";
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

void GestorCifrado::actualizarTriggersParaCifrado(const std::string& nombre_tabla, const std::map<std::string, std::string>& mapa_columnas) {
    if (gestor_db->getMotor() == GestorAuditoria::MotorDB::SQLite) {
        std::cout << "Advertencia: SQLite no soporta triggers de cifrado automatico." << std::endl;
        return;
    }

    std::cout << "Actualizando triggers para la tabla cifrada: " << nombre_tabla << std::endl;

    nlohmann::json datos;
    std::string tabla_original = nombre_tabla;
    if (boost::starts_with(tabla_original, "aud_")) {
        tabla_original = tabla_original.substr(4);
    }
    else if (boost::starts_with(tabla_original, "Aud")) {
        tabla_original = tabla_original.substr(3);
    }

    datos["tabla"] = tabla_original;
    datos["tabla_auditoria"] = nombre_tabla;
    datos["clave_hex"] = bytesAHex(clave.data(), clave.size());

    std::ostringstream columnas_cifradas_lista;
    std::ostringstream valores_cifrados_lista_new;
    std::ostringstream valores_cifrados_lista_old;
    bool first_col = true;

    for (const auto& par : mapa_columnas) {
        const std::string& col_original = par.first;
        const std::string& col_cifrada = par.second;

        if (!first_col) {
            columnas_cifradas_lista << ", ";
            valores_cifrados_lista_new << ", ";
            valores_cifrados_lista_old << ", ";
        }

        columnas_cifradas_lista << "\"" << col_cifrada << "\"";

        switch (gestor_db->getMotor()) {
        case GestorAuditoria::MotorDB::PostgreSQL:
            valores_cifrados_lista_new << "PGP_SYM_ENCRYPT(NEW.\"" << col_original << "\"::TEXT, '" << datos["clave_hex"].get<std::string>() << "')";
            valores_cifrados_lista_old << "PGP_SYM_ENCRYPT(OLD.\"" << col_original << "\"::TEXT, '" << datos["clave_hex"].get<std::string>() << "')";
            break;
        case GestorAuditoria::MotorDB::MySQL:
        case GestorAuditoria::MotorDB::SQLServer:
        case GestorAuditoria::MotorDB::SQLite:
            break;
        }
        first_col = false;
    }

    datos["lista_columnas_cifradas"] = columnas_cifradas_lista.str();
    datos["lista_valores_cifrados_new"] = valores_cifrados_lista_new.str();
    datos["lista_valores_cifrados_old"] = valores_cifrados_lista_old.str();

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
        return;
    }
    gestor_db->ejecutarComando(sql_comando);
}


bool GestorCifrado::esColumnaCifrada(const std::string& nombre_columna) {
    if (nombre_columna.empty()) return false;

    if (boost::iequals(nombre_columna, "UsuarioAccion") ||
        boost::iequals(nombre_columna, "FechaAccion") ||
        boost::iequals(nombre_columna, "AccionSql") ||
        boost::iequals(nombre_columna, "__temp_id")) {
        return false;
    }

    std::string posible_original = descifrarNombreColumnaCesar(nombre_columna, desplazamiento_cesar);

    return posible_original != nombre_columna;
}

void GestorCifrado::cifrarTablasDeAuditoria() {
    if (gestor_db->getMotor() == GestorAuditoria::MotorDB::SQLite) {
        std::cout << "Advertencia: SQLite no soporta cifrado automatico de tablas de auditoria." << std::endl;
        return;
    }

    auto tablas_auditoria = gestor_db->obtenerNombresDeTablas(true);
    tablas_auditoria.erase(std::remove_if(tablas_auditoria.begin(), tablas_auditoria.end(),
        [](const std::string& s) { return !boost::starts_with(s, "aud_"); }), tablas_auditoria.end());

    if (tablas_auditoria.empty()) {
        std::cout << "No se encontraron tablas de auditoria para cifrar." << std::endl;
        return;
    }

    for (const auto& tabla : tablas_auditoria) {
        std::cout << "Procesando tabla de auditoria: " << tabla << std::endl;

        auto resultado_columnas = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla + " LIMIT 0");
        std::vector<std::string> columnas_originales;

        for (const auto& col_cifrada : resultado_columnas.columnas) {
            if (esColumnaCifrada(col_cifrada)) {
                std::string col_original = descifrarNombreColumnaCesar(col_cifrada, desplazamiento_cesar);
                gestor_db->ejecutarComando("ALTER TABLE " + tabla + " RENAME COLUMN \"" + col_cifrada + "\" TO \"" + col_original + "\"");
                columnas_originales.push_back(col_original);
            }
            else {
                columnas_originales.push_back(col_cifrada);
            }
        }

        for (const auto& col_name : columnas_originales) {
            if (col_name != "UsuarioAccion" && col_name != "FechaAccion" && col_name != "AccionSql") {
                gestor_db->ejecutarComando("ALTER TABLE " + tabla + " ALTER COLUMN \"" + col_name + "\" TYPE TEXT");
            }
        }

        gestor_db->ejecutarComando("ALTER TABLE " + tabla + " ADD COLUMN __temp_id SERIAL PRIMARY KEY");
        auto datos_con_id = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla);

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
                if (col_name != "UsuarioAccion" && col_name != "FechaAccion" && col_name != "AccionSql") {
                    if (!first_set) update_sql += ", ";
                    std::string valor_a_cifrar = fila[i];
                    boost::replace_all(valor_a_cifrar, "'", "''");
                    update_sql += "\"" + col_name + "\" = '" + cifrarValor(valor_a_cifrar) + "'";
                    first_set = false;
                }
            }
            if (!first_set) {
                gestor_db->ejecutarComando(update_sql + where_clause);
            }
        }
        gestor_db->ejecutarComando("ALTER TABLE " + tabla + " DROP COLUMN __temp_id");

        auto columnas_finales = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla + " LIMIT 0");
        std::map<std::string, std::string> mapa_columnas;
        for (const auto& col : columnas_finales.columnas) {
            if (col != "UsuarioAccion" && col != "FechaAccion" && col != "AccionSql") {
                std::string cifrado_col = cifrarNombreColumnaCesar(col, desplazamiento_cesar);
                mapa_columnas[col] = cifrado_col;
                gestor_db->ejecutarComando("ALTER TABLE " + tabla + " RENAME COLUMN \"" + col + "\" TO \"" + cifrado_col + "\"");
            }
        }

        if (!mapa_columnas.empty()) {
            actualizarTriggersParaCifrado(tabla, mapa_columnas);
        }
    }
    std::cout << "Cifrado de tablas de auditoria completado." << std::endl;
}


std::vector<std::vector<std::string>> GestorCifrado::ejecutarConsultaConDesencriptado(const std::string& consulta) {
    auto resultado_cifrado = gestor_db->ejecutarConsultaConResultado(consulta);
    std::vector<std::vector<std::string>> resultado_final;

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