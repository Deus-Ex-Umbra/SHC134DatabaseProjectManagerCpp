#include "GestorSeguridad.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

GestorSeguridad::GestorSeguridad(GestorAuditoria* gestor, GestorAuditoria::MotorDB motor,
    const std::string& db, const std::string& user, const std::string& pass,
    const std::string& host, const std::string& port)
    : gestor_db(gestor), motor_db(motor), db_name(db), db_user(user), db_pass(pass), db_host(host), db_port(port)
{
    if (!gestor_db || !gestor_db->estaConectado()) {
        throw std::runtime_error("El GestorAuditoria no es valido o no esta conectado.");
    }
}

bool GestorSeguridad::exportarYEncriptarRespaldo(const std::string& ruta_archivo_salida, const std::string& clave, const std::string& iv) {
    if (clave.length() != 32 || iv.length() != 16) {
        std::cerr << "Error: La clave debe tener 32 caracteres (256 bits) y el IV 16 caracteres (128 bits)." << std::endl;
        return false;
    }

    std::string ruta_temporal = "respaldo_temporal.sql";
    std::string comando_dump;

    std::cout << "Iniciando exportacion completa de la base de datos..." << std::endl;

    switch (motor_db) {
    case GestorAuditoria::MotorDB::PostgreSQL:
        comando_dump = "set PGPASSWORD=" + db_pass + "&& pg_dump -h " + db_host + " -p " + db_port + " -U " + db_user + " -F p -f \"" + ruta_temporal + "\" " + db_name;
        break;
    case GestorAuditoria::MotorDB::MySQL:
        comando_dump = "mysqldump -h " + db_host + " -P " + db_port + " -u " + db_user + " -p" + db_pass + " --routines --triggers " + db_name + " > \"" + ruta_temporal + "\"";
        break;
    case GestorAuditoria::MotorDB::SQLServer:
        std::cerr << "Advertencia: La exportacion automatica para SQL Server no esta soportada. Se requiere un proceso manual con 'sqlcmd' o SSMS." << std::endl;
        return false;
    case GestorAuditoria::MotorDB::SQLite:
        comando_dump = "sqlite3 " + db_name + " .dump > " + ruta_temporal;
        break;
    }

    std::cout << "Ejecutando comando de respaldo: " << comando_dump << std::endl;
    int resultado = system(comando_dump.c_str());

    if (resultado != 0) {
        std::cerr << "Error: El comando de respaldo fallo con el codigo " << resultado << ". Asegurese de que las herramientas (pg_dump/mysqldump/sqlite3) esten en el PATH del sistema." << std::endl;
        return false;
    }

    std::cout << "Exportacion a archivo temporal completada." << std::endl;
    std::cout << "Iniciando encriptacion del respaldo..." << std::endl;

    bool exito = encriptarArchivo(ruta_temporal, ruta_archivo_salida,
        reinterpret_cast<const unsigned char*>(clave.c_str()),
        reinterpret_cast<const unsigned char*>(iv.c_str()));

    remove(ruta_temporal.c_str());

    if (exito) {
        std::cout << "Respaldo encriptado guardado en: " << ruta_archivo_salida << std::endl;
    }
    else {
        std::cerr << "Error durante la encriptacion del respaldo." << std::endl;
    }

    return exito;
}

bool GestorSeguridad::desencriptarRespaldo(const std::string& ruta_archivo_encriptado, const std::string& ruta_archivo_desencriptado, const std::string& clave, const std::string& iv) {
    if (clave.length() != 32 || iv.length() != 16) {
        std::cerr << "Error: La clave debe tener 32 caracteres (256 bits) y el IV 16 caracteres (128 bits)." << std::endl;
        return false;
    }

    std::cout << "Iniciando desencriptacion del respaldo..." << std::endl;
    bool exito = desencriptarArchivo(ruta_archivo_encriptado, ruta_archivo_desencriptado,
        reinterpret_cast<const unsigned char*>(clave.c_str()),
        reinterpret_cast<const unsigned char*>(iv.c_str()));

    if (exito) {
        std::cout << "Respaldo desencriptado guardado en: " << ruta_archivo_desencriptado << std::endl;
    }
    else {
        std::cerr << "Error durante la desencriptacion del respaldo." << std::endl;
    }
    return exito;
}

bool GestorSeguridad::encriptarArchivo(const std::string& ruta_entrada, const std::string& ruta_salida, const unsigned char* clave, const unsigned char* iv) {
    std::ifstream archivo_entrada(ruta_entrada, std::ios::binary);
    std::ofstream archivo_salida(ruta_salida, std::ios::binary);

    if (!archivo_entrada.is_open() || !archivo_salida.is_open()) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    unsigned char buffer_entrada[1024];
    unsigned char buffer_salida[1024 + AES_BLOCK_SIZE];
    int len_salida;

    do {
        archivo_entrada.read(reinterpret_cast<char*>(buffer_entrada), sizeof(buffer_entrada));
        std::streamsize bytes_leidos = archivo_entrada.gcount();
        if (bytes_leidos > 0) {
            if (1 != EVP_EncryptUpdate(ctx, buffer_salida, &len_salida, buffer_entrada, bytes_leidos)) {
                EVP_CIPHER_CTX_free(ctx);
                return false;
            }
            archivo_salida.write(reinterpret_cast<char*>(buffer_salida), len_salida);
        }
    } while (archivo_entrada.gcount() > 0);

    if (1 != EVP_EncryptFinal_ex(ctx, buffer_salida, &len_salida)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    archivo_salida.write(reinterpret_cast<char*>(buffer_salida), len_salida);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool GestorSeguridad::desencriptarArchivo(const std::string& ruta_entrada, const std::string& ruta_salida, const unsigned char* clave, const unsigned char* iv) {
    std::ifstream archivo_entrada(ruta_entrada, std::ios::binary);
    std::ofstream archivo_salida(ruta_salida, std::ios::binary);

    if (!archivo_entrada.is_open() || !archivo_salida.is_open()) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, clave, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    unsigned char buffer_entrada[1024];
    unsigned char buffer_salida[1024 + AES_BLOCK_SIZE];
    int len_salida;

    do {
        archivo_entrada.read(reinterpret_cast<char*>(buffer_entrada), sizeof(buffer_entrada));
        std::streamsize bytes_leidos = archivo_entrada.gcount();
        if (bytes_leidos > 0) {
            if (1 != EVP_DecryptUpdate(ctx, buffer_salida, &len_salida, buffer_entrada, bytes_leidos)) {
                EVP_CIPHER_CTX_free(ctx);
                return false;
            }
            archivo_salida.write(reinterpret_cast<char*>(buffer_salida), len_salida);
        }
    } while (archivo_entrada.gcount() > 0);

    if (1 != EVP_DecryptFinal_ex(ctx, buffer_salida, &len_salida)) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    archivo_salida.write(reinterpret_cast<char*>(buffer_salida), len_salida);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}