#pragma once
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include "Modelos.hpp"
#include "GestorAuditoria.hpp"

namespace po = boost::program_options;

void ejecutarComando(const std::string& comando, bool esperar = true);
void imprimirRutasApi(const std::vector<Tabla>& tablas, const std::string& dir_salida);

void manejarScaffolding(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion);
void manejarAuditoria(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion);
void manejarEncriptado(const po::variables_map& vm, GestorAuditoria::MotorDB motor, const std::string& info_conexion);