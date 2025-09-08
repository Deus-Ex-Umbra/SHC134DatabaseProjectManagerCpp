#include "GestorEncriptacion.hpp"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <stdexcept>

GestorEncriptacion::GestorEncriptacion(const std::vector<unsigned char>& key) : clave(key) {
    if (clave.size() != 32) {
        throw std::runtime_error("La clave de encriptacion debe ser de 256 bits (32 bytes).");
    }
}

std::vector<unsigned char> GestorEncriptacion::encriptar(const std::string& texto_plano) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de cifrado.");

    std::vector<unsigned char> texto_cifrado(texto_plano.length() + AES_BLOCK_SIZE);
    int len;
    int ciphertext_len;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al inicializar el cifrado.");
    }

    if (1 != EVP_EncryptUpdate(ctx, texto_cifrado.data(), &len, reinterpret_cast<const unsigned char*>(texto_plano.c_str()), texto_plano.length())) {
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
    texto_cifrado.resize(ciphertext_len);
    return texto_cifrado;
}

std::string GestorEncriptacion::desencriptar(const std::vector<unsigned char>& texto_cifrado) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Fallo al crear el contexto de descifrado.");

    std::vector<unsigned char> texto_plano(texto_cifrado.size());
    int len;
    int plaintext_len;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave.data(), NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al inicializar el descifrado.");
    }

    if (1 != EVP_DecryptUpdate(ctx, texto_plano.data(), &len, texto_cifrado.data(), texto_cifrado.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al actualizar el descifrado.");
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, texto_plano.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Fallo al finalizar el descifrado. La clave puede ser incorrecta.");
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return std::string(reinterpret_cast<char*>(texto_plano.data()), plaintext_len);
}