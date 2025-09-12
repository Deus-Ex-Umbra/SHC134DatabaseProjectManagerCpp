# SHC134 Database Project Manager

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Version](https://img.shields.io/badge/version-1.0.0-green.svg)

Una potente herramienta de línea de comandos para automatizar tareas complejas de bases de datos, incluyendo generación de proyectos API con Nest.js, configuración de auditorías y cifrado de datos sensibles con AES-256.

## ✨ Características

- 🏗️ **Scaffolding**: Genera estructura completa de proyecto API con Nest.js y TypeORM
- 📝 **Auditoría**: Crea tablas de auditoría y triggers automáticos para registro de cambios
- 🔒 **Encriptado**: Cifrado César para nombres de columnas y AES-256 para datos
- 🔍 **Consultas Seguras**: Ejecuta consultas SQL con descifrado automático de resultados
- 🔌 **Multi-motor**: Soporte completo para PostgreSQL, MySQL, SQL Server y SQLite

## 📋 Requisitos Previos

### Ejecutable
- Archivo `SHC134DatabaseProjectManagerCpp.exe` compilado y accesible desde tu terminal
- Windows con soporte para aplicaciones C++ compiladas

### Drivers de Base de Datos
- **MySQL**: Requiere MySQL ODBC 8.0 Unicode Driver
- **SQL Server**: Requiere ODBC Driver 17 for SQL Server
- **PostgreSQL**: Cliente libpq incluido generalmente con PostgreSQL
- **SQLite**: SQLite3 ODBC Driver

### Node.js y npm
- Node.js v16 o superior para ejecutar el proyecto API generado
- npm o yarn para gestión de paquetes

## 🚀 Uso General

La herramienta se invoca desde la terminal con la siguiente estructura:
```bash
.\SHC134DatabaseProjectManagerCpp.exe <accion> [opciones]
Opciones Comunes de Conexión
OpciónDescripciónValor por Defecto--motorMotor de BD (postgres, mysql, sqlserver, sqlite)postgres--hostDirección del servidorlocalhost--portPuerto del servidorVaría según motor--dbnameNombre de la base de datosRequerido--userUsuario para conexiónpostgres--passwordContraseña del usuarioVacío

Nota: Para SQLite solo se requiere --dbname con la ruta al archivo de base de datos.

🏗️ Scaffolding
Genera una estructura completa de proyecto API con Nest.js a partir del esquema de una base de datos existente, incluyendo:

Módulos CRUD para cada tabla
Autenticación JWT automática si detecta tabla de usuarios
DTOs con validación
Servicios con TypeORM
Controladores RESTful
Dependencias específicas según motor de BD

Opciones Específicas
OpciónDescripciónRequerido--outDirectorio de salida del proyectoNo (default: api-generada-nest)--jwt-secretClave secreta para tokens JWTSí
Ejemplos
PostgreSQL (con Docker):
bash.\SHC134DatabaseProjectManagerCpp.exe scaffolding --motor postgres --host localhost --port 5432 --dbname nest_db --user root --password "root" --jwt-secret "MI_CLAVE_SECRETA_SUPER_SEGURA_123" --out mi-api-postgres
MySQL (con Docker):
bash.\SHC134DatabaseProjectManagerCpp.exe scaffolding --motor mysql --host localhost --port 3306 --dbname nest_db --user root --password "root" --jwt-secret "MI_CLAVE_SECRETA_SUPER_SEGURA_123" --out mi-api-mysql
SQL Server (con Docker):
bash.\SHC134DatabaseProjectManagerCpp.exe scaffolding --motor sqlserver --host localhost --port 1433 --dbname nest_db --user sa --password "Abcd1234" --jwt-secret "MI_CLAVE_SECRETA_SUPER_SEGURA_123" --out mi-api-sqlserver
SQLite:
bash.\SHC134DatabaseProjectManagerCpp.exe scaffolding --motor sqlite --dbname "C:\databases\mi_db.sqlite" --jwt-secret "MI_CLAVE_SECRETA_SUPER_SEGURA_123"
Estructura del Proyecto Generado
mi-api/
├── src/
│   ├── main.ts
│   ├── app.module.ts
│   ├── database/
│   │   └── typeorm.config.ts
│   ├── autenticacion/           # Si hay tabla de usuarios
│   │   ├── auth.module.ts
│   │   ├── auth.controller.ts
│   │   ├── auth.service.ts
│   │   ├── dto/
│   │   │   └── login.dto.ts
│   │   ├── estrategias/
│   │   │   └── jwt.strategy.ts
│   │   └── guardianes/
│   │       └── jwt-auth.guard.ts
│   └── [nombre-tabla]/           # Para cada tabla
│       ├── entidades/
│       │   └── [nombre].entity.ts
│       ├── dto/
│       │   ├── crear-[nombre].dto.ts
│       │   └── actualizar-[nombre].dto.ts
│       ├── [nombre].service.ts
│       ├── [nombre].controller.ts
│       └── [nombre].module.ts
├── package.json                  # Con dependencias específicas del motor
├── tsconfig.json
└── .env                          # Configuración de BD y JWT
📝 Auditoría
Crea tablas de auditoría (prefijo aud_) y triggers que registran automáticamente todas las operaciones INSERT, UPDATE y DELETE.
Opciones Específicas
OpciónDescripciónRequerido--tablaAudita una tabla específicaNo (audita todas por defecto)
Ejemplos
PostgreSQL - Auditar todas las tablas:
bash.\SHC134DatabaseProjectManagerCpp.exe auditoria --motor postgres --host localhost --port 5432 --dbname nest_db --user root --password "root"
MySQL - Auditar tabla específica:
bash.\SHC134DatabaseProjectManagerCpp.exe auditoria --motor mysql --host localhost --port 3306 --dbname nest_db --user root --password "root" --tabla usuarios
SQL Server - Auditar todas las tablas:
bash.\SHC134DatabaseProjectManagerCpp.exe auditoria --motor sqlserver --host localhost --port 1433 --dbname nest_db --user sa --password "Abcd1234"
Verificación en Base de Datos
PostgreSQL:
bashdocker exec -it postgres_db psql -U root -d nest_db
\dt aud_*
MySQL:
bashdocker exec -it mysql_db mysql -u root -p nest_db
SHOW TABLES LIKE 'aud_%';
SQL Server:
bashdocker exec -it sqlserver_db /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P "Abcd1234" -d nest_db -N -C
SELECT name FROM sys.tables WHERE name LIKE 'aud_%';
GO
🔒 Encriptado
Gestiona el cifrado de las tablas de auditoría utilizando:

Cifrado César: Para nombres de columnas (basado en el primer carácter de la clave)
AES-256-CBC: Para todos los datos (actuales y futuros)

Opciones Específicas
OpciónDescripciónRequerido--keyClave hexadecimal de 64 caracteres (32 bytes)Sí--encrypt-audit-tablesCifra todas las tablas de auditoríaSí (para cifrar)--queryEjecuta consulta SQL con descifradoSí (para consultar)
Generación de Clave Segura
Para generar una clave hexadecimal de 64 caracteres:
PowerShell:
powershell-join ((1..32) | ForEach {'{0:X2}' -f (Get-Random -Max 256)})
Python:
pythonimport secrets
print(secrets.token_hex(32))
Ejemplos de Cifrado
PostgreSQL:
bash.\SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --host localhost --port 5432 --dbname nest_db --user root --password "root" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --encrypt-audit-tables
MySQL:
bash.\SHC134DatabaseProjectManagerCpp.exe encriptado --motor mysql --host localhost --port 3306 --dbname nest_db --user root --password "root" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --encrypt-audit-tables
SQL Server:
bash.\SHC134DatabaseProjectManagerCpp.exe encriptado --motor sqlserver --host localhost --port 1433 --dbname nest_db --user sa --password "Abcd1234" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --encrypt-audit-tables
Consultas con Descifrado
PostgreSQL - Ver datos descifrados:
bash.\SHC134DatabaseProjectManagerCpp.exe sql --motor postgres --host localhost --port 5432 --dbname nest_db --user root --password "root" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --query "SELECT * FROM aud_usuarios LIMIT 5"
MySQL - Consulta específica:
bash.\SHC134DatabaseProjectManagerCpp.exe sql --motor mysql --host localhost --port 3306 --dbname nest_db --user root --password "root" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --query "SELECT * FROM aud_productos WHERE AccionSql = 'Modificado'"
SQL Server - Análisis de auditoría:
bash.\SHC134DatabaseProjectManagerCpp.exe sql --motor sqlserver --host localhost --port 1433 --dbname nest_db --user sa --password "Abcd1234" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --query "SELECT TOP 10 * FROM aud_ventas ORDER BY FechaAccion DESC"
🔍 Consultas SQL
Ejecuta consultas SQL directas con soporte opcional para descifrado de datos.
Consultas Sin Cifrado
bash.\SHC134DatabaseProjectManagerCpp.exe sql --motor postgres --host localhost --port 5432 --dbname nest_db --user root --password "root" --query "SELECT COUNT(*) FROM usuarios"
Consultas Con Descifrado
bash.\SHC134DatabaseProjectManagerCpp.exe sql --motor mysql --host localhost --port 3306 --dbname nest_db --user root --password "root" --key "TU_CLAVE_HEX_64_CHARS" --query "SELECT * FROM aud_clientes"
🔧 Flujo de Trabajo Completo
1. Crear Base de Datos y Tablas
sql-- Ejemplo para PostgreSQL
CREATE TABLE usuarios (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    nombre VARCHAR(100)
);

CREATE TABLE productos (
    id SERIAL PRIMARY KEY,
    nombre VARCHAR(255) NOT NULL,
    precio DECIMAL(10,2),
    usuario_id INTEGER REFERENCES usuarios(id)
);
2. Generar Auditoría
bash.\SHC134DatabaseProjectManagerCpp.exe auditoria --motor postgres --dbname nest_db --user root --password "root"
3. Cifrar Tablas de Auditoría
bash.\SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --dbname nest_db --user root --password "root" --key "A1B2C3D4E5F6789012345678901234567890ABCDEF123456789012345678ABCD" --encrypt-audit-tables
4. Generar API
bash.\SHC134DatabaseProjectManagerCpp.exe scaffolding --motor postgres --dbname nest_db --user root --password "root" --jwt-secret "MI_JWT_SECRET" --out mi-api
5. Ejecutar API
bashcd mi-api
npm install
npm run start:dev
📊 Seguridad
Mejores Prácticas

Claves de Cifrado:

Usar claves aleatorias de 64 caracteres hexadecimales
Almacenar claves en un gestor de secretos seguro
Nunca compartir o versionar claves


JWT Secrets:

Usar secretos largos y complejos (mínimo 32 caracteres)
Rotar regularmente los secretos JWT
Diferentes secretos para cada ambiente


Conexiones a BD:

Usar SSL/TLS cuando sea posible
Limitar permisos del usuario de BD
Usar contraseñas fuertes


Auditoría:

Revisar regularmente los logs de auditoría
Mantener respaldos de las tablas de auditoría
Monitorear accesos no autorizados



🐛 Solución de Problemas
Error: "BLOB/TEXT column used in key specification"
Solución: El sistema maneja automáticamente los índices en MySQL durante el cifrado.
Error: "Driver ODBC no encontrado"
Solución: Instalar el driver ODBC correspondiente:

MySQL: MySQL ODBC Connector
SQL Server: ODBC Driver for SQL Server
SQLite: SQLite ODBC Driver

La API no se conecta a la base de datos
Verificar:

Archivo .env con credenciales correctas
Servicio de base de datos activo
Puerto no bloqueado por firewall
Dependencias npm instaladas correctamente