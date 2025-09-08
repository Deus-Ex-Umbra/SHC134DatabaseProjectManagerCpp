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
        if (resultado.columnas.empty() || resultado.filas.empty()) {
            std::cout << "La tabla esta vacia, solo se cifraran las columnas." << std::endl;
        }

        for (const auto& fila : resultado.filas) {
            std::string update_sql = "UPDATE " + tabla + " SET ";
            std::string where_clause = " WHERE ";
            bool first_set = true;
            bool first_where = true;

            for (size_t i = 0; i < resultado.columnas.size(); ++i) {
                if (resultado.columnas[i] != "UsuarioAccion" && resultado.columnas[i] != "FechaAccion" && resultado.columnas[i] != "AccionSql") {
                    if (!first_set) update_sql += ", ";
                    update_sql += resultado.columnas[i] + " = '" + cifrarValor(fila[i]) + "'";
                    first_set = false;
                }
                if (!first_where) where_clause += " AND ";
                where_clause += resultado.columnas[i] + " = '" + fila[i] + "'";
                first_where = false;
            }
            if (!first_set) gestor_db->ejecutarComando(update_sql + where_clause);
        }

        for (const auto& col : resultado.columnas) {
            if (col != "UsuarioAccion" && col != "FechaAccion" && col != "AccionSql") {
                std::string cifrado_col = cifrarValor(col);
                std::string rename_sql = "ALTER TABLE " + tabla + " RENAME COLUMN " + col + " TO \"" + cifrado_col + "\"";
                gestor_db->ejecutarComando(rename_sql);
            }
        }
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