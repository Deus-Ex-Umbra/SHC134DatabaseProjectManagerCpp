#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include "GestorAuditoria.hpp"
#include <inja/inja.hpp>

class GestorAuditoria;

class GestorCifrado {
public:
    GestorCifrado(std::shared_ptr<GestorAuditoria> gestor, const std::string& clave_encriptacion_hex);

    // Métodos principales de cifrado
    void cifrarTablasDeAuditoria();
    void cifrarFilaEInsertar(const std::string& tabla, const std::vector<std::string>& columnas,
        const std::vector<std::string>& fila, const std::string& accion);
    std::vector<std::vector<std::string>> ejecutarConsultaConDesencriptado(const std::string& consulta);
    std::string getClave() const;

private:
    std::shared_ptr<GestorAuditoria> gestor_db;
    std::vector<unsigned char> clave;
    std::string clave_hex;
    int desplazamiento_cesar;
    inja::Environment env_plantillas;

    // Métodos de cifrado/descifrado
    std::string cifrarValor(const std::string& texto_plano);
    std::string descifrarValor(const std::string& texto_cifrado_hex);

    // Métodos para manejo de bases de datos específicas
    void prepararCifradoSQLServer();
    void actualizarTriggersParaCifrado(const std::string& nombre_tabla,
        const std::map<std::string, std::string>& mapa_columnas);

    // Métodos auxiliares
    bool esColumnaCifrada(const std::string& nombre_columna);
};