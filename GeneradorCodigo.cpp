#include "GeneradorCodigo.hpp"
#include <inja/inja.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

GeneradorCodigo::GeneradorCodigo(const std::string& dir_salida) : dir_salida(dir_salida) {
}

void GeneradorCodigo::escribirArchivo(const std::string& ruta, const std::string& contenido) {
    fs::path ruta_archivo(ruta);
    if (!ruta_archivo.parent_path().empty()) {
        fs::create_directories(ruta_archivo.parent_path());
    }
    std::ofstream archivo(ruta_archivo);
    archivo << contenido;
}

std::string GeneradorCodigo::renderizarPlantilla(const std::string& ruta_plantilla, const json& datos) {
    inja::Environment env;
    try {
        return env.render_file(ruta_plantilla, datos);
    }
    catch (const std::exception& e) {
        std::cerr << "Error Critico al renderizar la plantilla " << ruta_plantilla << ": " << e.what() << std::endl;
        return "";
    }
}

void GeneradorCodigo::generarArchivosBase(const json& datos_modulos) {
    std::cout << "Generando archivos base del proyecto..." << std::endl;

    std::string contenido_main_ts = R"(import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';
import { ValidationPipe } from '@nestjs/common';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  app.useGlobalPipes(new ValidationPipe());
  app.enableCors();
  await app.listen(3000);
  console.log(`La aplicacion se esta ejecutando en: ${await app.getUrl()}`);
}
bootstrap();
)";

    std::string contenido_tsconfig_json = R"({
  "compilerOptions": {
    "module": "commonjs",
    "declaration": true,
    "removeComments": true,
    "emitDecoratorMetadata": true,
    "experimentalDecorators": true,
    "allowSyntheticDefaultImports": true,
    "esModuleInterop": true,
    "moduleResolution": "node",
    "target": "ES2021",
    "sourceMap": true,
    "outDir": "./dist",
    "baseUrl": "./",
    "incremental": true,
    "skipLibCheck": true,
    "strictNullChecks": false,
    "noImplicitAny": false,
    "strictBindCallApply": false,
    "forceConsistentCasingInFileNames": false,
    "noFallthroughCasesInSwitch": false
  }
}
)";

    escribirArchivo(dir_salida + "/package.json", renderizarPlantilla("PackageJson.tpl", {}));
    escribirArchivo(dir_salida + "/src/main.ts", contenido_main_ts);
    escribirArchivo(dir_salida + "/src/database/typeorm.config.ts", renderizarPlantilla("TypeOrmConfig.tpl", {}));
    escribirArchivo(dir_salida + "/src/app.module.ts", renderizarPlantilla("AppModule.tpl", datos_modulos));

    std::cout << "Generando archivos de configuracion..." << std::endl;
    escribirArchivo(dir_salida + "/tsconfig.json", contenido_tsconfig_json);
    std::string gitignore_content = "node_modules\n.env\ndist\n";
    escribirArchivo(dir_salida + "/.gitignore", gitignore_content);
}

void GeneradorCodigo::generarModuloAutenticacion(const Tabla& tabla_usuario) {
    std::cout << "Generando modulo de autenticacion para la tabla: " << tabla_usuario.nombre << std::endl;
    fs::create_directories(dir_salida + "/src/autenticacion/dto");
    fs::create_directories(dir_salida + "/src/autenticacion/estrategias");
    fs::create_directories(dir_salida + "/src/autenticacion/guardianes");

    json datos;
    datos["moduloUsuario"]["nombreClaseModulo"] = tabla_usuario.nombre_clase + "Module";
    datos["moduloUsuario"]["nombreClaseServicio"] = tabla_usuario.nombre_clase + "Service";
    datos["moduloUsuario"]["nombreClaseEntidad"] = tabla_usuario.nombre_clase;
    datos["moduloUsuario"]["nombreCarpeta"] = tabla_usuario.nombre_archivo;
    datos["moduloUsuario"]["nombreArchivo"] = tabla_usuario.nombre_archivo;
    datos["moduloUsuario"]["campo_email"] = tabla_usuario.campo_email_encontrado;
    datos["moduloUsuario"]["campo_contrasena"] = tabla_usuario.campo_contrasena_encontrado;
    datos["moduloUsuario"]["clave_primaria"]["nombre_camel_case"] = tabla_usuario.clave_primaria.nombre_camel_case;

    escribirArchivo(dir_salida + "/src/autenticacion/auth.module.ts", renderizarPlantilla("AuthModule.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/auth.controller.ts", renderizarPlantilla("AuthController.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/auth.service.ts", renderizarPlantilla("AuthService.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/estrategias/jwt.strategy.ts", renderizarPlantilla("JwtStrategy.tpl", {}));

    json datos_login_dto;
    datos_login_dto["campo_email"] = tabla_usuario.campo_email_encontrado;
    datos_login_dto["campo_contrasena"] = tabla_usuario.campo_contrasena_encontrado;

    std::string contenido_login_dto = "export class LoginDto {\n"
        "  " + tabla_usuario.campo_email_encontrado + ": string;\n"
        "  " + tabla_usuario.campo_contrasena_encontrado + ": string;\n"
        "}";
    escribirArchivo(dir_salida + "/src/autenticacion/dto/login.dto.ts", contenido_login_dto);

    std::string jwt_guard = "import { Injectable } from '@nestjs/common';\nimport { AuthGuard } from '@nestjs/passport';\n\n@Injectable()\nexport class JwtAuthGuard extends AuthGuard('jwt') {}";
    escribirArchivo(dir_salida + "/src/autenticacion/guardianes/jwt-auth.guard.ts", jwt_guard);
}

void GeneradorCodigo::generarModuloCrud(const Tabla& tabla) {
    std::cout << "Generando CRUD para la tabla: " << tabla.nombre << std::endl;
    std::string ruta_modulo = dir_salida + "/src/" + tabla.nombre_archivo;
    fs::create_directories(ruta_modulo + "/entidades");
    fs::create_directories(ruta_modulo + "/dto");

    json datos_plantilla;
    json& datos_tabla = datos_plantilla["tabla"];
    datos_tabla["nombre"] = tabla.nombre;
    datos_tabla["nombre_clase"] = tabla.nombre_clase;
    datos_tabla["nombre_variable"] = tabla.nombre_variable;
    datos_tabla["nombre_archivo"] = tabla.nombre_archivo;
    datos_tabla["es_tabla_usuario"] = tabla.es_tabla_usuario;
    datos_tabla["es_protegida"] = tabla.es_protegida;
    datos_tabla["clave_primaria"]["nombre_camel_case"] = tabla.clave_primaria.nombre_camel_case;

    if (tabla.es_tabla_usuario) {
        datos_tabla["campo_email"] = tabla.campo_email_encontrado;
        datos_tabla["campo_contrasena"] = tabla.campo_contrasena_encontrado;
    }

    for (const auto& col : tabla.columnas) {
        json col_data;
        col_data["nombre"] = col.nombre;
        col_data["nombre_camel_case"] = col.nombre_camel_case;
        col_data["tipo_db"] = col.tipo_db;
        col_data["tipo_ts"] = col.tipo_ts;
        col_data["es_nulo"] = col.es_nulo;
        col_data["es_pk"] = col.es_pk;
        std::string decorador_tipo;
        if (col.tipo_ts == "string") {
            decorador_tipo = "@IsString()";
        }
        else if (col.tipo_ts == "number") {
            decorador_tipo = "@IsNumber()";
        }
        else if (col.tipo_ts == "boolean") {
            decorador_tipo = "@IsBoolean()";
        }
        else if (col.tipo_ts == "Date") {
            decorador_tipo = "@IsDate()";
        }
        else {
            decorador_tipo = "";
        }
        col_data["decorador_tipo"] = decorador_tipo;

        datos_tabla["columnas"].push_back(col_data);
    }

    escribirArchivo(ruta_modulo + "/entidades/" + tabla.nombre_archivo + ".entity.ts", renderizarPlantilla("Entity.tpl", datos_plantilla));
    escribirArchivo(ruta_modulo + "/dto/crear-" + tabla.nombre_archivo + ".dto.ts", renderizarPlantilla("CreateDto.tpl", datos_plantilla));
    escribirArchivo(ruta_modulo + "/dto/actualizar-" + tabla.nombre_archivo + ".dto.ts", renderizarPlantilla("UpdateDto.tpl", datos_plantilla));
    escribirArchivo(ruta_modulo + "/" + tabla.nombre_archivo + ".service.ts", renderizarPlantilla("Service.tpl", datos_plantilla));
    escribirArchivo(ruta_modulo + "/" + tabla.nombre_archivo + ".controller.ts", renderizarPlantilla("Controller.tpl", datos_plantilla));
    escribirArchivo(ruta_modulo + "/" + tabla.nombre_archivo + ".module.ts", renderizarPlantilla("Module.tpl", datos_plantilla));
}

void GeneradorCodigo::generarProyectoCompleto(const std::vector<Tabla>& tablas, const std::string& motor_db, const std::string& host, const std::string& puerto, const std::string& usuario, const std::string& contrasena, const std::string& base_datos, const std::string& jwt_secret) {
    const Tabla* ptr_tabla_usuario = nullptr;
    json datos_modulos;

    std::cout << "=== INICIANDO GENERACION DE PROYECTO ===" << std::endl;
    std::cout << "Tablas encontradas: " << tablas.size() << std::endl;

    for (const auto& tabla : tablas) {
        std::cout << "Procesando tabla: " << tabla.nombre
            << " (Usuario: " << (tabla.es_tabla_usuario ? "SI" : "NO")
            << ", Protegida: " << (tabla.es_protegida ? "SI" : "NO") << ")" << std::endl;

        json mod;
        mod["nombreClaseModulo"] = tabla.nombre_clase + "Module";
        mod["nombreCarpeta"] = tabla.nombre_archivo;
        mod["nombreArchivo"] = tabla.nombre_archivo;
        datos_modulos["modulos"].push_back(mod);

        if (tabla.es_tabla_usuario) {
            ptr_tabla_usuario = &tabla;
            std::cout << "  -> Tabla de usuario identificada" << std::endl;
            std::cout << "     Email: '" << tabla.campo_email_encontrado << "'" << std::endl;
            std::cout << "     Contraseña: '" << tabla.campo_contrasena_encontrado << "'" << std::endl;
        }
    }

    if (ptr_tabla_usuario) {
        std::cout << "Agregando modulo de autenticacion..." << std::endl;
        json mod_auth;
        mod_auth["nombreClaseModulo"] = "AuthModule";
        mod_auth["nombreCarpeta"] = "autenticacion";
        mod_auth["nombreArchivo"] = "auth";
        datos_modulos["modulos"].push_back(mod_auth);
    }
    else {
        std::cout << "ADVERTENCIA: No se generar autenticacion - no hay tabla de usuario valida" << std::endl;
    }

    generarArchivosBase(datos_modulos);

    if (ptr_tabla_usuario) {
        try {
            generarModuloAutenticacion(*ptr_tabla_usuario);
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR generando modulo de autenticacion: " << e.what() << std::endl;
            throw;
        }
    }

    for (const auto& tabla : tablas) {
        generarModuloCrud(tabla);
    }
    generarArchivoEnv(motor_db, host, puerto, usuario, contrasena, base_datos, jwt_secret);
}

void GeneradorCodigo::generarArchivoEnv(const std::string& motor_db, const std::string& host, const std::string& puerto, const std::string& usuario, const std::string& contrasena, const std::string& base_datos, const std::string& jwt_secret) {
    std::cout << "Generando archivo .env con configuraciones de base de datos..." << std::endl;

    std::string tipo_db;
    if (motor_db == "postgres") tipo_db = "postgres";
    else if (motor_db == "mysql") tipo_db = "mysql";
    else if (motor_db == "sqlserver") tipo_db = "mssql";
    else if (motor_db == "sqlite") tipo_db = "sqlite";
    else tipo_db = "postgres";

    std::string contenido_env = "# Configuracion de Base de Datos\n";
    contenido_env += "DB_TYPE=" + tipo_db + "\n";
    contenido_env += "DB_HOST=" + host + "\n";
    contenido_env += "DB_PORT=" + puerto + "\n";
    contenido_env += "DB_USERNAME=" + usuario + "\n";
    contenido_env += "DB_PASSWORD=\"" + contrasena + "\"\n";
    contenido_env += "DB_DATABASE=" + base_datos + "\n\n";
    contenido_env += "# Configuracion JWT\n";
    contenido_env += "JWT_SECRET=\"" + jwt_secret + "\"\n\n";
    contenido_env += "# Configuracion de Aplicacion\n";
    contenido_env += "NODE_ENV=development\n";
    contenido_env += "PORT=3000\n";

    escribirArchivo(dir_salida + "/.env", contenido_env);

    std::cout << "Archivo .env generado con:" << std::endl;
    std::cout << "  Tipo BD: " << tipo_db << std::endl;
    std::cout << "  Host: " << host << ":" << puerto << std::endl;
    std::cout << "  Base de datos: " << base_datos << std::endl;
    std::cout << "  Usuario: " << usuario << std::endl;
}