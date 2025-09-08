#pragma once
#include <string>
#include <vector>
#include "Modelos.hpp"

void ejecutarComando(const std::string& comando, bool esperar = true);
void imprimirRutasApi(const std::vector<Tabla>& tablas, const std::string& dir_salida);