#include "GestorCifrado.hpp"
#include "GestorAuditoria.hpp"
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
    if (hex.length() % 2 != 0) {
        throw std::invalid_argument("La cadena hexadecimal debe tener una longitud par.");
    }
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
    const std::string caracteres_permitidos =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_";

    std::string resultado;
    resultado.reserve(texto_plano.length());

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
    const std::string caracteres_permitidos =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "_";

    std::string resultado;
    resultado.reserve(texto_cifrado.length());

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
        throw std::runtime_error("La clave de encriptacion debe ser una cadena hexadecimal de 64 caracteres (32 bytes).");
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
    else {
        desplazamiento_cesar = 0;
    }
}

void GestorCifrado::cifrarFilaEInsertar(const std::string& tabla, const std::vector<std::string>& columnas, const std::vector<std::string>& fila, const std::string& accion) {
    std::string tabla_auditoria = "aud_" + tabla;
    std::ostringstream insert_sql;
    std::ostringstream values_sql;

    insert_sql << "INSERT INTO " << tabla_auditoria << " (";
    values_sql << ") VALUES (";

    bool first = true;
    for (size_t i = 0; i < columnas.size(); ++i) {
        if (!first) {
            insert_sql << ", ";
            values_sql << ", ";
        }

        insert_sql << "\"" << cifrarNombreColumnaCesar(columnas[i], desplazamiento_cesar) << "\"";

        std::string valor_cifrado = cifrarValor(fila[i]);
        boost::replace_all(valor_cifrado, "'", "''");
        values_sql << "'" << valor_cifrado << "'";

        first = false;
    }

    insert_sql << ", \"" << cifrarNombreColumnaCesar("UsuarioAccion", desplazamiento_cesar) << "\", \"" << cifrarNombreColumnaCesar("FechaAccion", desplazamiento_cesar) << "\", \"" << cifrarNombreColumnaCesar("AccionSql", desplazamiento_cesar) << "\"";

    std::string valor_usuario_cifrado = cifrarValor("SYSTEM");
    boost::replace_all(valor_usuario_cifrado, "'", "''");
    std::string valor_accion_cifrado = cifrarValor(accion);
    boost::replace_all(valor_accion_cifrado, "'", "''");

    values_sql << ", '" << valor_usuario_cifrado << "', '" << cifrarValor("datetime('now')") << "', '" << valor_accion_cifrado << "')";

    gestor_db->ejecutarComando(insert_sql.str() + values_sql.str());
}

std::string GestorCifrado::cifrarValor(const std::string& texto_plano) {
    if (texto_plano.empty() || texto_plano == "NULL") {
        return texto_plano;
    }

    unsigned char iv[AES_BLOCK_SIZE];
    if (!RAND_bytes(iv, sizeof(iv))) {
        throw std::runtime_error("Error al generar el IV aleatorio para AES.");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de cifrado EVP.");

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), iv);

    std::vector<unsigned char> texto_cifrado(texto_plano.length() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, texto_cifrado.data(), &len, reinterpret_cast<const unsigned char*>(texto_plano.c_str()), texto_plano.length());
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
        return std::string(reinterpret_cast<char*>(texto_plano.data()), plaintext_len);
    }
    catch (const std::exception&) {
        return texto_cifrado_hex;
    }
}

void GestorCifrado::cifrarTablasDeAuditoria() {
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

    if (gestor_db->getMotor() == GestorAuditoria::MotorDB::SQLServer) {
        prepararCifradoSQLServer();
    }

    for (const auto& tabla : tablas_auditoria) {
        try {
            std::cout << "Procesando tabla " << tabla << "..." << std::endl;

            if (gestor_db->getMotor() == GestorAuditoria::MotorDB::MySQL) {
                eliminarIndicesMySQL(tabla);
            }

            auto resultado_columnas = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla + " LIMIT 1");
            std::map<std::string, std::string> mapa_columnas;

            for (const auto& col : resultado_columnas.columnas) {
                mapa_columnas[col] = cifrarNombreColumnaCesar(col, desplazamiento_cesar);
                std::string alter_sql;
                switch (gestor_db->getMotor()) {
                case GestorAuditoria::MotorDB::PostgreSQL:
                    alter_sql = "ALTER TABLE public.\"" + tabla + "\" ALTER COLUMN \"" + col + "\" TYPE TEXT";
                    break;
                case GestorAuditoria::MotorDB::MySQL:
                    alter_sql = "ALTER TABLE `" + tabla + "` MODIFY COLUMN `" + col + "` TEXT";
                    break;
                case GestorAuditoria::MotorDB::SQLServer:
                    alter_sql = "ALTER TABLE dbo.[" + tabla + "] ALTER COLUMN [" + col + "] NVARCHAR(MAX)";
                    break;
                default: continue;
                }
                gestor_db->ejecutarComando(alter_sql);
            }

            if (mapa_columnas.empty()) continue;

            auto datos_actuales = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + tabla);
            if (datos_actuales.filas.empty()) {
                std::cout << "Tabla " << tabla << " sin datos, renombrando columnas..." << std::endl;
                for (const auto& par : mapa_columnas) {
                    std::string rename_sql;
                    switch (gestor_db->getMotor()) {
                    case GestorAuditoria::MotorDB::PostgreSQL:
                        rename_sql = "ALTER TABLE public.\"" + tabla + "\" RENAME COLUMN \"" + par.first + "\" TO \"" + par.second + "\"";
                        break;
                    case GestorAuditoria::MotorDB::MySQL:
                        rename_sql = "ALTER TABLE `" + tabla + "` RENAME COLUMN `" + par.first + "` TO `" + par.second + "`";
                        break;
                    case GestorAuditoria::MotorDB::SQLServer:
                        rename_sql = "EXEC sp_rename '" + tabla + "." + par.first + "', '" + par.second + "', 'COLUMN'";
                        break;
                    default: continue;
                    }
                    gestor_db->ejecutarComando(rename_sql);
                }
                continue;
            }

            if (gestor_db->getMotor() == GestorAuditoria::MotorDB::MySQL) {
                gestor_db->ejecutarComando("CREATE TEMPORARY TABLE temp_" + tabla + " AS SELECT * FROM " + tabla);
                gestor_db->ejecutarComando("TRUNCATE TABLE " + tabla);
            }

            for (const auto& fila : datos_actuales.filas) {
                std::ostringstream insert_sql;
                std::ostringstream values_sql;
                insert_sql << "INSERT INTO " << tabla << " (";
                values_sql << ") VALUES (";

                bool first = true;
                for (size_t i = 0; i < datos_actuales.columnas.size(); ++i) {
                    if (!first) {
                        insert_sql << ", ";
                        values_sql << ", ";
                    }

                    std::string col_name_quoted;
                    switch (gestor_db->getMotor()) {
                    case GestorAuditoria::MotorDB::PostgreSQL:
                        col_name_quoted = "\"" + datos_actuales.columnas[i] + "\"";
                        break;
                    case GestorAuditoria::MotorDB::MySQL:
                        col_name_quoted = "`" + datos_actuales.columnas[i] + "`";
                        break;
                    case GestorAuditoria::MotorDB::SQLServer:
                        col_name_quoted = "[" + datos_actuales.columnas[i] + "]";
                        break;
                    default:
                        col_name_quoted = datos_actuales.columnas[i];
                    }

                    insert_sql << col_name_quoted;
                    std::string valor_cifrado = cifrarValor(fila[i]);
                    boost::replace_all(valor_cifrado, "'", "''");
                    values_sql << "'" << valor_cifrado << "'";
                    first = false;
                }

                gestor_db->ejecutarComando(insert_sql.str() + values_sql.str());
            }

            for (const auto& par : mapa_columnas) {
                std::string rename_sql;
                switch (gestor_db->getMotor()) {
                case GestorAuditoria::MotorDB::PostgreSQL:
                    rename_sql = "ALTER TABLE public.\"" + tabla + "\" RENAME COLUMN \"" + par.first + "\" TO \"" + par.second + "\"";
                    break;
                case GestorAuditoria::MotorDB::MySQL:
                    rename_sql = "ALTER TABLE `" + tabla + "` RENAME COLUMN `" + par.first + "` TO `" + par.second + "`";
                    break;
                case GestorAuditoria::MotorDB::SQLServer:
                    rename_sql = "EXEC sp_rename '" + tabla + "." + par.first + "', '" + par.second + "', 'COLUMN'";
                    break;
                default: continue;
                }
                gestor_db->ejecutarComando(rename_sql);
            }

            std::string nombre_tabla_original = tabla;
            boost::replace_first(nombre_tabla_original, "aud_", "");
            boost::replace_first(nombre_tabla_original, "Aud", "");
            actualizarTriggersParaCifrado(nombre_tabla_original, mapa_columnas);

            std::cout << "Tabla " << tabla << " cifrada exitosamente." << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error procesando tabla " << tabla << ": " << e.what() << std::endl;
        }
    }
}

void GestorCifrado::eliminarIndicesMySQL(const std::string& tabla) {
    try {
        auto resultado = gestor_db->ejecutarConsultaConResultado(
            "SELECT DISTINCT INDEX_NAME FROM INFORMATION_SCHEMA.STATISTICS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + tabla + "' "
            "AND INDEX_NAME != 'PRIMARY'"
        );

        for (const auto& fila : resultado.filas) {
            if (!fila.empty()) {
                gestor_db->ejecutarComando("DROP INDEX `" + fila[0] + "` ON `" + tabla + "`");
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Advertencia al eliminar indices: " << e.what() << std::endl;
    }
}

std::vector<std::vector<std::string>> GestorCifrado::ejecutarConsultaConDesencriptado(const std::string& consulta) {
    auto resultado_cifrado = gestor_db->ejecutarConsultaConResultado(consulta);
    std::vector<std::vector<std::string>> resultado_final;

    if (resultado_cifrado.filas.empty()) return resultado_final;

    std::vector<std::string> cabeceras_descifradas;
    for (const auto& col : resultado_cifrado.columnas) {
        cabeceras_descifradas.push_back(descifrarNombreColumnaCesar(col, desplazamiento_cesar));
    }
    resultado_final.push_back(cabeceras_descifradas);

    for (const auto& fila : resultado_cifrado.filas) {
        std::vector<std::string> fila_descifrada;
        for (size_t i = 0; i < fila.size(); ++i) {
            fila_descifrada.push_back(descifrarValor(fila[i]));
        }
        resultado_final.push_back(fila_descifrada);
    }
    return resultado_final;
}

void GestorCifrado::prepararCifradoSQLServer() {
    try {
        gestor_db->ejecutarComando("CREATE MASTER KEY ENCRYPTION BY PASSWORD = 'DevPasswordComplexEnough#123!'");
    }
    catch (const std::exception& e) {
    }

    std::string preparacion_sql =
        "IF NOT EXISTS (SELECT 1 FROM sys.certificates WHERE name = 'AuditoriaCert')\n"
        "BEGIN\n"
        "    CREATE CERTIFICATE AuditoriaCert WITH SUBJECT = 'Auditoria Certificate';\n"
        "END\n"
        "GO\n"
        "IF NOT EXISTS (SELECT 1 FROM sys.symmetric_keys WHERE name = 'AuditoriaKey')\n"
        "BEGIN\n"
        "    CREATE SYMMETRIC KEY AuditoriaKey WITH ALGORITHM = AES_256 ENCRYPTION BY CERTIFICATE AuditoriaCert;\n"
        "END\n"
        "GO";

    gestor_db->ejecutarComando(preparacion_sql);
}

void GestorCifrado::actualizarTriggersParaCifrado(const std::string& nombre_tabla_original, const std::map<std::string, std::string>& mapa_columnas) {
    nlohmann::json datos;
    datos["tabla"] = nombre_tabla_original;
    datos["tabla_auditoria"] = "aud_" + nombre_tabla_original;
    datos["clave_hex"] = clave_hex;

    std::ostringstream columnas_cifradas_lista, valores_insert_new, valores_update_old, valores_delete_old;
    bool first = true;

    auto resultado_columnas_original = gestor_db->ejecutarConsultaConResultado("SELECT * FROM " + nombre_tabla_original + " LIMIT 1");
    std::vector<std::string> nombres_columnas_original = resultado_columnas_original.columnas;

    for (const auto& col_name : nombres_columnas_original) {
        if (!first) {
            columnas_cifradas_lista << ", ";
            valores_insert_new << ", ";
            valores_update_old << ", ";
            valores_delete_old << ", ";
        }

        std::string cast_str, quote_open, quote_close, record_prefix_new, record_prefix_old;
        switch (gestor_db->getMotor()) {
        case GestorAuditoria::MotorDB::PostgreSQL:
            cast_str = "::TEXT"; quote_open = "\""; quote_close = "\""; record_prefix_new = "NEW."; record_prefix_old = "OLD."; break;
        case GestorAuditoria::MotorDB::MySQL:
            cast_str = ""; quote_open = "`"; quote_close = "`"; record_prefix_new = "NEW."; record_prefix_old = "OLD."; break;
        case GestorAuditoria::MotorDB::SQLServer:
            cast_str = ")"; quote_open = "["; quote_close = "]"; record_prefix_new = "i."; record_prefix_old = "d."; break;
        default: break;
        }

        columnas_cifradas_lista << quote_open << cifrarNombreColumnaCesar(col_name, desplazamiento_cesar) << quote_close;

        if (gestor_db->getMotor() == GestorAuditoria::MotorDB::SQLServer) {
            valores_insert_new << "EncryptByKey(Key_GUID('AuditoriaKey'), CAST(" << record_prefix_new << quote_open << col_name << quote_close << " AS NVARCHAR(MAX))" << cast_str;
            valores_update_old << "EncryptByKey(Key_GUID('AuditoriaKey'), CAST(" << record_prefix_old << quote_open << col_name << quote_close << " AS NVARCHAR(MAX))" << cast_str;
            valores_delete_old << "EncryptByKey(Key_GUID('AuditoriaKey'), CAST(" << record_prefix_old << quote_open << col_name << quote_close << " AS NVARCHAR(MAX))" << cast_str;
        }
        else {
            valores_insert_new << "encrypt_val(" << record_prefix_new << quote_open << col_name << quote_close << ")";
            valores_update_old << "encrypt_val(" << record_prefix_old << quote_open << col_name << quote_close << ")";
            valores_delete_old << "encrypt_val(" << record_prefix_old << quote_open << col_name << quote_close << ")";
        }

        first = false;
    }

    std::string quote_open, quote_close;
    switch (gestor_db->getMotor()) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        quote_open = "\""; quote_close = "\""; break;
    case GestorAuditoria::MotorDB::MySQL:
        quote_open = "`"; quote_close = "`"; break;
    case GestorAuditoria::MotorDB::SQLServer:
        quote_open = "["; quote_close = "]"; break;
    default: break;
    }

    columnas_cifradas_lista << ", " << quote_open << cifrarNombreColumnaCesar("UsuarioAccion", desplazamiento_cesar) << quote_close
        << ", " << quote_open << cifrarNombreColumnaCesar("FechaAccion", desplazamiento_cesar) << quote_close
        << ", " << quote_open << cifrarNombreColumnaCesar("AccionSql", desplazamiento_cesar) << quote_close;

    switch (gestor_db->getMotor()) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        valores_insert_new << ", encrypt_val(SESSION_USER), encrypt_val(NOW()::TEXT), encrypt_val('Insertado')";
        valores_update_old << ", encrypt_val(SESSION_USER), encrypt_val(NOW()::TEXT), encrypt_val('Modificado')";
        valores_delete_old << ", encrypt_val(SESSION_USER), encrypt_val(NOW()::TEXT), encrypt_val('Eliminado')";
        break;
    case GestorAuditoria::MotorDB::MySQL:
        valores_insert_new << ", encrypt_val(SUBSTRING_INDEX(CURRENT_USER(),'@',1)), encrypt_val(NOW()), encrypt_val('Insertado')";
        valores_update_old << ", encrypt_val(SUBSTRING_INDEX(CURRENT_USER(),'@',1)), encrypt_val(NOW()), encrypt_val('Modificado')";
        valores_delete_old << ", encrypt_val(SUBSTRING_INDEX(CURRENT_USER(),'@',1)), encrypt_val(NOW()), encrypt_val('Eliminado')";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        valores_insert_new << ", EncryptByKey(Key_GUID('AuditoriaKey'), CAST(SUSER_SNAME() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST(GETDATE() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST('Insertado' AS NVARCHAR(MAX)))";
        valores_update_old << ", EncryptByKey(Key_GUID('AuditoriaKey'), CAST(SUSER_SNAME() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST(GETDATE() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST('Modificado' AS NVARCHAR(MAX)))";
        valores_delete_old << ", EncryptByKey(Key_GUID('AuditoriaKey'), CAST(SUSER_SNAME() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST(GETDATE() AS NVARCHAR(MAX))), EncryptByKey(Key_GUID('AuditoriaKey'), CAST('Eliminado' AS NVARCHAR(MAX)))";
        break;
    default: break;
    }

    datos["lista_columnas_cifradas"] = columnas_cifradas_lista.str();
    datos["valores_insert_new"] = valores_insert_new.str();
    datos["valores_update_old"] = valores_update_old.str();
    datos["valores_delete_old"] = valores_delete_old.str();

    std::string plantilla_a_usar;
    switch (gestor_db->getMotor()) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        plantilla_a_usar = "PostgresAuditCifrado.tpl";
        break;
    case GestorAuditoria::MotorDB::MySQL:
        plantilla_a_usar = "MySqlAuditCifrado.tpl";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        plantilla_a_usar = "SqlServerAuditCifrado.tpl";
        break;
    default: return;
    }

    std::string sql_comando = env_plantillas.render_file(plantilla_a_usar, datos);
    gestor_db->ejecutarComando(sql_comando);
}

std::string GestorCifrado::getClave() const {
    return clave_hex;
}