#include "GestorCifrado.hpp"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>

// Función auxiliar para convertir de Hex a Bytes
std::vector<unsigned char> hexABytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// Función auxiliar para convertir de Bytes a Hex
std::string bytesAHex(const unsigned char* data, int len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    return ss.str();
}

GestorCifrado::GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion)
    : gestor_db(gestor) {
    if (clave_encriptacion.length() != 32) {
        throw std::runtime_error("La clave de encriptacion debe tener 32 caracteres.");
    }
    clave.assign(clave_encriptacion.begin(), clave_encriptacion.end());
}

std::string GestorCifrado::encriptar(const std::string& texto_plano) {
    unsigned char iv[AES_BLOCK_SIZE];
    if (!RAND_bytes(iv, sizeof(iv))) {
        throw std::runtime_error("Error al generar el IV aleatorio.");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de cifrado.");

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), iv)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al inicializar el cifrado.");
    }

    std::vector<unsigned char> texto_cifrado(texto_plano.length() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertext_len = 0;

    if (1 != EVP_EncryptUpdate(ctx, texto_cifrado.data(), &len, (const unsigned char*)texto_plano.c_str(), texto_plano.length())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al actualizar el cifrado.");
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, texto_cifrado.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al finalizar el cifrado.");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    std::string iv_hex = bytesAHex(iv, AES_BLOCK_SIZE);
    std::string cifrado_hex = bytesAHex(texto_cifrado.data(), ciphertext_len);

    return iv_hex + cifrado_hex;
}

std::string GestorCifrado::desencriptar(const std::string& texto_cifrado_hex) {
    if (texto_cifrado_hex.length() < 32) { // 16 bytes de IV en formato hex
        throw std::runtime_error("El texto cifrado es invalido.");
    }

    std::vector<unsigned char> iv = hexABytes(texto_cifrado_hex.substr(0, 32));
    std::vector<unsigned char> texto_cifrado = hexABytes(texto_cifrado_hex.substr(32));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de descifrado.");

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al inicializar el descifrado.");
    }

    std::vector<unsigned char> texto_plano(texto_cifrado.size() + AES_BLOCK_SIZE);
    int len = 0;
    int plaintext_len = 0;

    if (1 != EVP_DecryptUpdate(ctx, texto_plano.data(), &len, texto_cifrado.data(), texto_cifrado.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al actualizar el descifrado.");
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, texto_plano.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al finalizar el descifrado. Verifique la clave.");
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)texto_plano.data(), plaintext_len);
}

void GestorCifrado::cifrarTabla(const std::string& nombre_tabla, const std::vector<std::string>& nombres_columnas) {
    std::cout << "El cifrado a nivel de base de datos mediante triggers no es una practica recomendada." << std::endl;
    std::cout << "Se recomienda realizar el cifrado a nivel de aplicacion antes de insertar/actualizar los datos." << std::endl;
    std::cout << "Esta funcion solo sirve como demostracion y no debe usarse en produccion." << std::endl;
}

void GestorCifrado::descifrarTabla(const std::string& nombre_tabla, const std::vector<std::string>& nombres_columnas) {
    std::cout << "El descifrado masivo de tablas no esta implementado. Use la ejecucion de consultas para ver datos descifrados." << std::endl;
}

std::vector<std::vector<std::string>> GestorCifrado::ejecutarConsultaYDescifrar(
    const std::string& consulta,
    const std::vector<std::string>& alias_columnas_cifradas
) {
    auto resultados_cifrados = gestor_db->ejecutarConsultaConResultado(consulta);
    if (resultados_cifrados.empty()) {
        return resultados_cifrados;
    }

    // Suponiendo que la primera fila nos da los nombres/alias de las columnas
    // Esta parte es compleja sin metadatos del resultado. Asumiremos que el usuario sabe lo que hace.
    std::vector<int> indices_a_descifrar;
    // La forma de obtener los nombres de las columnas varía mucho. Lo simplificamos aquí.
    // En un caso real, necesitaríamos analizar la consulta o usar una librería que devuelva metadatos.

    // Este enfoque es una SIMPLIFICACION. Se asume que el usuario lista los alias en el orden correcto.
    for (const auto& alias : alias_columnas_cifradas) {
        // Lógica para encontrar el índice de la columna 'alias'. 
        // Por ahora, es un placeholder. El usuario debe garantizar el orden.
    }


    for (auto& fila : resultados_cifrados) {
        for (size_t i = 0; i < fila.size(); ++i) { // Recorremos todas las columnas
            try {
                // Intentamos descifrar cada campo. Si falla, lo dejamos como está.
                fila[i] = desencriptar(fila[i]);
            }
            catch (const std::exception& e) {
                // No hacer nada, el dato no estaba encriptado o la clave es incorrecta
            }
        }
    }

    return resultados_cifrados;
}