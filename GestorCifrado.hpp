#pragma once
#include <string>
#include <vector>
#include <memory>
#include "GestorAuditoria.hpp"
#include <inja/inja.hpp>

class GestorAuditoria;

class GestorCifrado {
public:
    GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion_hex);

    void cifrarTablasDeAuditoria();
    void cifrarFilaEInsertar(const std::string& tabla, const std::vector<std::string>& columnas, const std::vector<std::string>& fila, const std::string& accion);
    std::vector<std::vector<std::string>> ejecutarConsultaConDesencriptado(
        const std::string& consulta
    );

private:
    std::shared_ptr<GestorAuditoria> gestor_db;
    std::vector<unsigned char> clave;
    inja::Environment env_plantillas;

    std::string cifrarValor(const std::string& texto_plano);
    std::string descifrarValor(const std::string& texto_cifrado_hex);
    void prepararCifradoSQLServer();
    void actualizarTriggersParaCifrado(const std::string& nombre_tabla, const std::vector<std::string>& columnas_originales);
};