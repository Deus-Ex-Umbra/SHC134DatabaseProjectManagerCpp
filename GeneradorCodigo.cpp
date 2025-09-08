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

    std::string login_dto = "export class LoginDto {\n  " + tabla_usuario.campo_email_encontrado + ": string;\n  " + tabla_usuario.campo_contrasena_encontrado + ": string;\n}";
    escribirArchivo(dir_salida + "/src/autenticacion/dto/login.dto.ts", login_dto);

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

void GeneradorCodigo::generarProyectoCompleto(const std::vector<Tabla>& tablas) {
    const Tabla* ptr_tabla_usuario = nullptr;
    json datos_modulos;

    for (const auto& tabla : tablas) {
        json mod;
        mod["nombreClaseModulo"] = tabla.nombre_clase + "Module";
        mod["nombreCarpeta"] = tabla.nombre_archivo;
        mod["nombreArchivo"] = tabla.nombre_archivo;
        datos_modulos["modulos"].push_back(mod);
        if (tabla.es_tabla_usuario) {
            ptr_tabla_usuario = &tabla;
        }
    }

    if (ptr_tabla_usuario) {
        json mod_auth;
        mod_auth["nombreClaseModulo"] = "AuthModule";
        mod_auth["nombreCarpeta"] = "autenticacion";
        mod_auth["nombreArchivo"] = "auth";
        datos_modulos["modulos"].push_back(mod_auth);
    }

    generarArchivosBase(datos_modulos);

    if (ptr_tabla_usuario) {
        generarModuloAutenticacion(*ptr_tabla_usuario);
    }

    for (const auto& tabla : tablas) {
        generarModuloCrud(tabla);
    }

    std::cout << "\nGeneracion de codigo completa. El proyecto se encuentra en el directorio '" << dir_salida << "'." << std::endl;
}