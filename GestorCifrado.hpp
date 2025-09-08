#pragma once
#include <string>
#include <vector>
#include <memory>
#include "GestorAuditoria.hpp"

class GestorCifrado {
public:
    GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion);

    void cifrarTabla(const std::string& nombre_tabla, const std::vector<std::string>& nombres_columnas);
    void descifrarTabla(const std::string& nombre_tabla, const std::vector<std::string>& nombres_columnas);

    std::vector<std::vector<std::string>> ejecutarConsultaYDescifrar(
        const std::string& consulta,
        const std::vector<std::string>& alias_columnas_cifradas
    );

private:
    std::shared_ptr<GestorAuditoria> gestor_db;
    std::vector<unsigned char> clave;

    std::string encriptar(const std::string& texto_plano);
    std::string desencriptar(const std::string& texto_cifrado_hex);
};