#pragma once
#include <string>
#include <vector>

class GestorEncriptacion {
public:
    GestorEncriptacion(const std::vector<unsigned char>& key);
    std::vector<unsigned char> encriptar(const std::string& texto_plano);
    std::string desencriptar(const std::vector<unsigned char>& texto_cifrado);

private:
    std::vector<unsigned char> clave;
};