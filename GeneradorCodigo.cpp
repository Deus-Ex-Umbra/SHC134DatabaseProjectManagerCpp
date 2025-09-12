#include "GeneradorCodigo.hpp"
#include "Utils.hpp"
#include <inja/inja.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <map>

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

void GeneradorCodigo::generarPackageJson(const std::string& motor_db) {
    json datos_package;
    datos_package["nombre"] = "generated-api";
    datos_package["version"] = "0.0.1";

    std::string dependencias_base = R"({
  "name": "generated-api",
  "version": "0.0.1",
  "private": true,
  "scripts": {
    "build": "nest build",
    "start": "nest start",
    "start:dev": "nest start --watch",
    "start:prod": "node dist/main"
  },
  "dependencies": {
    "@nestjs/common": "^10.0.0",
    "@nestjs/core": "^10.0.0",
    "@nestjs/config": "^3.2.2",
    "@nestjs/jwt": "^10.2.0",
    "@nestjs/mapped-types": "^2.0.5",
    "@nestjs/passport": "^10.0.3",
    "@nestjs/platform-express": "^10.0.0",
    "@nestjs/typeorm": "^10.0.2",
    "bcrypt": "^5.1.1",
    "class-transformer": "^0.5.1",
    "class-validator": "^0.14.1",
    "passport": "^0.7.0",
    "passport-jwt": "^4.0.1",)";

    std::string dependencias_db;
    if (motor_db == "postgres") {
        dependencias_db = R"(
    "pg": "^8.7.3",)";
    }
    else if (motor_db == "mysql") {
        dependencias_db = R"(
    "mysql2": "^3.6.5",)";
    }
    else if (motor_db == "mssql" || motor_db == "sqlserver") {
        dependencias_db = R"(
    "mssql": "^10.0.1",)";
    }
    else if (motor_db == "sqlite") {
        dependencias_db = R"(
    "sqlite3": "^5.1.6",)";
    }

    std::string dependencias_resto = R"(
    "reflect-metadata": "^0.1.13",
    "rxjs": "^7.8.1",
    "typeorm": "^0.3.17"
  },
  "devDependencies": {
    "@nestjs/cli": "^10.0.0",
    "@types/express": "^4.17.17",
    "@types/node": "^20.3.1",
    "@types/bcrypt": "^5.0.2",
    "@types/passport-jwt": "^4.0.1",
    "source-map-support": "^0.5.21",
    "ts-loader": "^9.4.3",
    "ts-node": "^10.9.1",
    "tsconfig-paths": "^4.2.0",
    "typescript": "^5.1.3"
  }
})";

    std::string contenido_completo = dependencias_base + dependencias_db + dependencias_resto;
    escribirArchivo(dir_salida + "/package.json", contenido_completo);
}

void GeneradorCodigo::generarArchivosBase(const json& datos_modulos, const std::string& motor_db) {
    std::cout << "Generando archivos base del proyecto..." << std::endl;
    std::string contenido_main_ts = R"(import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';
import { ValidationPipe } from '@nestjs/common';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  app.useGlobalPipes(new ValidationPipe({ whitelist: false, forbidNonWhitelisted: false }));
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
})";
    generarPackageJson(motor_db);
    escribirArchivo(dir_salida + "/src/main.ts", contenido_main_ts);
    escribirArchivo(dir_salida + "/src/database/typeorm.config.ts", renderizarPlantilla("TypeOrmConfig.tpl", {}));
    escribirArchivo(dir_salida + "/src/app.module.ts", renderizarPlantilla("AppModule.tpl", datos_modulos));
    std::cout << "Generando archivos de configuracion..." << std::endl;
    escribirArchivo(dir_salida + "/tsconfig.json", contenido_tsconfig_json);
    escribirArchivo(dir_salida + "/.gitignore", "node_modules\n.env\ndist\n");
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
    datos["moduloUsuario"]["clave_primaria"]["nombre"] = tabla_usuario.clave_primaria.nombre;
    escribirArchivo(dir_salida + "/src/autenticacion/auth.module.ts", renderizarPlantilla("AuthModule.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/auth.controller.ts", renderizarPlantilla("AuthController.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/auth.service.ts", renderizarPlantilla("AuthService.tpl", datos));
    escribirArchivo(dir_salida + "/src/autenticacion/estrategias/jwt.strategy.ts", renderizarPlantilla("JwtStrategy.tpl", {}));
    escribirArchivo(dir_salida + "/src/autenticacion/dto/login.dto.ts", "export class LoginDto {\n  " + tabla_usuario.campo_email_encontrado + ": string;\n  " + tabla_usuario.campo_contrasena_encontrado + ": string;\n}");
    escribirArchivo(dir_salida + "/src/autenticacion/guardianes/jwt-auth.guard.ts", "import { Injectable } from '@nestjs/common';\nimport { AuthGuard } from '@nestjs/passport';\n\n@Injectable()\nexport class JwtAuthGuard extends AuthGuard('jwt') {}");
}

void GeneradorCodigo::generarModuloCrud(const Tabla& tabla, const std::vector<Tabla>& todas_las_tablas) {
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
    datos_tabla["clave_primaria"]["nombre"] = tabla.clave_primaria.nombre;
    datos_tabla["campo_email"] = tabla.campo_email_encontrado;
    datos_tabla["campo_contrasena"] = tabla.campo_contrasena_encontrado;

    for (const auto& col : tabla.columnas) {
        json col_data;
        col_data["nombre"] = col.nombre;
        col_data["tipo_ts"] = col.tipo_ts;
        col_data["es_nulo"] = col.es_nulo;
        col_data["es_pk"] = col.es_pk;
        col_data["es_fk"] = col.es_fk;
        col_data["tipo_db"] = col.tipo_db;
        col_data["decorador_tipo"] = (col.tipo_ts == "string") ? "@IsString()" : (col.tipo_ts == "number") ? "@IsNumber()" : (col.tipo_ts == "boolean") ? "@IsBoolean()" : (col.tipo_ts == "Date") ? "@IsDate()" : "";
        datos_tabla["columnas"].push_back(col_data);
    }

    json dependencias_imports = json::array();
    json dependencias_relaciones = json::array();
    std::set<std::string> clases_importadas;
    std::set<std::string> columnas_fk_procesadas;

    for (const auto& fk : tabla.dependencias_fk) {
        if (columnas_fk_procesadas.find(fk.columna_local) != columnas_fk_procesadas.end()) {
            continue;
        }
        columnas_fk_procesadas.insert(fk.columna_local);

        for (const auto& tabla_ref : todas_las_tablas) {
            if (tabla_ref.nombre == fk.tabla_referenciada) {
                json fk_data;
                fk_data["columna_local"] = fk.columna_local;
                fk_data["clase_tabla_referenciada"] = tabla_ref.nombre_clase;
                fk_data["variable_tabla_referenciada"] = aCamelCase(fk.columna_local);
                fk_data["archivo_tabla_referenciada"] = tabla_ref.nombre_archivo;

                dependencias_relaciones.push_back(fk_data);

                if (clases_importadas.find(tabla_ref.nombre_clase) == clases_importadas.end()) {
                    dependencias_imports.push_back(fk_data);
                    clases_importadas.insert(tabla_ref.nombre_clase);
                }
                break;
            }
        }
    }

    datos_tabla["dependencias_imports"] = dependencias_imports;
    datos_tabla["dependencias_relaciones"] = dependencias_relaciones;

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
        std::cout << "Procesando tabla: " << tabla.nombre << " (Usuario: " << (tabla.es_tabla_usuario ? "SI" : "NO") << ", Protegida: " << (tabla.es_protegida ? "SI" : "NO") << ")" << std::endl;
        json mod;
        mod["nombreClaseModulo"] = tabla.nombre_clase + "Module";
        mod["nombreCarpeta"] = tabla.nombre_archivo;
        mod["nombreArchivo"] = tabla.nombre_archivo;
        datos_modulos["modulos"].push_back(mod);
        if (tabla.es_tabla_usuario) ptr_tabla_usuario = &tabla;
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
        std::cout << "ADVERTENCIA: No se generara autenticacion - no hay tabla de usuario valida" << std::endl;
    }
    generarArchivosBase(datos_modulos, motor_db);
    if (ptr_tabla_usuario) {
        generarModuloAutenticacion(*ptr_tabla_usuario);
    }
    for (const auto& tabla : tablas) {
        generarModuloCrud(tabla, tablas);
    }
    generarArchivoEnv(motor_db, host, puerto, usuario, contrasena, base_datos, jwt_secret);
}

void GeneradorCodigo::generarArchivoEnv(const std::string& motor_db, const std::string& host, const std::string& puerto, const std::string& usuario, const std::string& contrasena, const std::string& base_datos, const std::string& jwt_secret) {
    std::cout << "Generando archivo .env con configuraciones de base de datos..." << std::endl;
    std::string tipo_db = (motor_db == "postgres") ? "postgres" : (motor_db == "mysql") ? "mysql" : (motor_db == "sqlserver" || motor_db == "mssql") ? "mssql" : (motor_db == "sqlite") ? "sqlite" : "postgres";
    std::string usuario_db = (tipo_db == "mssql") ? "sa" : usuario;
    std::string contenido_env = "# Configuracion de Base de Datos\n";
    contenido_env += "DB_TYPE=" + tipo_db + "\n";
    contenido_env += "DB_HOST=" + host + "\n";
    contenido_env += "DB_PORT=" + puerto + "\n";
    contenido_env += "DB_USERNAME=" + usuario_db + "\n";
    contenido_env += "DB_PASSWORD=\"" + contrasena + "\"\n";
    contenido_env += "DB_DATABASE=" + base_datos + "\n\n";
    contenido_env += "# Configuracion JWT\n";
    contenido_env += "JWT_SECRET=\"" + jwt_secret + "\"\n\n";
    contenido_env += "# Configuracion de Aplicacion\n";
    contenido_env += "NODE_ENV=development\n";
    contenido_env += "PORT=3000\n";
    escribirArchivo(dir_salida + "/.env", contenido_env);
}