#pragma once
#include <string>
#include <vector>
#include <memory>
#include "GestorAuditoria.hpp"

class GestorCifrado {
public:
    GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion_hex);

    void cifrarTablasDeAuditoria();

    std::vector<std::vector<std::string>> ejecutarConsultaConDesencriptado(
        const std::string& consulta
    );

private:
    std::shared_ptr<GestorAuditoria> gestor_db;
    std::vector<unsigned char> clave;

    std::string cifrarValor(const std::string& texto_plano);
    std::string descifrarValor(const std::string& texto_cifrado_hex);
};