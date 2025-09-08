# SHC134 Database Project Manager

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Version](https://img.shields.io/badge/version-1.0.0-green.svg)

Una potente herramienta de línea de comandos para automatizar tareas complejas de bases de datos, incluyendo generación de proyectos API, configuración de auditorías, cifrado de datos sensibles y exportación de respaldos.

## ✨ Características

- 🏗️ **Scaffolding**: Genera estructura completa de proyecto API con Nest.js
- 📝 **Auditoría**: Crea tablas y triggers de auditoría automática
- 🔒 **Encriptado**: Gestiona cifrado de tablas de auditoría
- 📤 **Exportación**: Realiza respaldos completos de bases de datos
- 🔌 **Multi-motor**: Soporte para PostgreSQL, MySQL, SQL Server y SQLite

## 📋 Requisitos Previos

### Ejecutable
Asegúrate de tener el archivo `SHC134DatabaseProjectManagerCpp.exe` compilado y accesible desde tu terminal.

### Drivers de Base de Datos
- **MySQL/SQL Server**: Requieren drivers ODBC instalados en Windows
- **PostgreSQL/SQLite**: Generalmente no requieren instalaciones adicionales

## 🚀 Uso General

La herramienta se invoca desde la terminal con la siguiente estructura:

```bash
SHC134DatabaseProjectManagerCpp.exe <accion> [opciones]
```

### Opciones Comunes de Conexión

| Opción | Alias | Descripción | Valor por Defecto |
|--------|-------|-------------|-------------------|
| `--motor` | | Motor de BD (postgres, mysql, sqlserver, sqlite) | `postgres` |
| `--host` | | Dirección del servidor | `localhost` |
| `--port` | | Puerto del servidor | `5432` |
| `--dbname` | | Nombre de la base de datos | `nest_db` |
| `--user` | | Usuario para conexión | `root` |
| `--password` | | Contraseña del usuario | `root` |

> **Nota**: Las opciones `--host`, `--port`, `--user` y `--password` no son necesarias para SQLite.

## 🏗️ Scaffolding

Genera una estructura completa de proyecto API con Nest.js a partir del esquema de una base de datos existente, incluyendo autenticación JWT.

### Opciones Específicas

| Opción | Alias | Descripción | Requerido |
|--------|-------|-------------|-----------|
| `--out` | `-o` | Directorio de salida del proyecto | No |
| `--jwt-secret` | | Clave secreta para tokens JWT | Sí |

### Ejemplos

**PostgreSQL:**
```bash
SHC134DatabaseProjectManagerCpp.exe scaffolding --motor postgres --host localhost --port 5432 --dbname mi_db --user admin --password "pass" --jwt-secret "MI_CLAVE_SECRETA" --out mi-api
```

**MySQL:**
```bash
SHC134DatabaseProjectManagerCpp.exe scaffolding --motor mysql --host 127.0.0.1 --port 3306 --dbname mi_db --user root --password "pass" --jwt-secret "MI_CLAVE_SECRETA"
```

**SQLite:**
```bash
SHC134DatabaseProjectManagerCpp.exe scaffolding --motor sqlite --dbname "C:\ruta\a\mi\db.sqlite" --jwt-secret "MI_CLAVE_SECRETA"
```

## 📝 Auditoría

Crea tablas de auditoría (tablas espejo) y los triggers necesarios para registrar automáticamente las operaciones UPDATE y DELETE.

### Opciones Específicas

| Opción | Alias | Descripción | Requerido |
|--------|-------|-------------|-----------|
| `--tabla` | | Audita una tabla específica (omite para auditar todas) | No |

### Ejemplos

**Auditar todas las tablas (PostgreSQL):**
```bash
SHC134DatabaseProjectManagerCpp.exe auditoria --motor postgres --dbname mi_db --user admin --password "pass"
```

**Auditar tabla específica (MySQL):**
```bash
SHC134DatabaseProjectManagerCpp.exe auditoria --motor mysql --dbname mi_db --user root --password "pass" --tabla clientes
```

## 🔒 Encriptado

Gestiona el cifrado de las tablas de auditoría. Puede cifrar datos existentes y modificar triggers, o ejecutar consultas descifrando resultados.

### Opciones Específicas

| Opción | Alias | Descripción | Requerido |
|--------|-------|-------------|-----------|
| `--key` | `-k` | Clave de cifrado hexadecimal (64 caracteres) | Sí |
| `--encrypt-audit-tables` | | Cifra todas las tablas de auditoría (aud_*) | No |
| `--query` | `-q` | Ejecuta consulta SQL mostrando resultados descifrados | No |

> **Nota**: Usar `--encrypt-audit-tables` O `--query`, pero no ambos simultáneamente.

### Ejemplos

**Cifrar tablas de auditoría:**
```bash
SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --dbname mi_db --user admin --password "pass" --key "TU_CLAVE_HEXADECIMAL_DE_64_CARACTERES_AQUI" --encrypt-audit-tables
```

**Consultar datos descifrados:**
```bash
SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --dbname mi_db --user admin --password "pass" --key "TU_CLAVE_HEXADECIMAL_DE_64_CARACTERES_AQUI" --query "SELECT * FROM aud_usuarios LIMIT 10"
```

## 📤 Exportar

Realiza un respaldo completo de la base de datos (tablas, rutinas y triggers) en un único archivo SQL.

### Opciones Específicas

| Opción | Alias | Descripción | Requerido |
|--------|-------|-------------|-----------|
| `--out` | `-o` | Ruta del archivo de respaldo | Sí |

> ⚠️ **Advertencia**: La exportación para SQL Server no está soportada y debe realizarse manualmente desde SQL Server Management Studio.

### Ejemplos

**Exportar PostgreSQL:**
```bash
SHC134DatabaseProjectManagerCpp.exe exportar --motor postgres --host localhost --dbname mi_db --user admin --password "pass" --out "C:\Respaldos\mi_db_backup.sql"
```

**Exportar MySQL:**
```bash
SHC134DatabaseProjectManagerCpp.exe exportar --motor mysql --user root --password "pass" --dbname mi_db --out "respaldo_mysql.sql"
```

**Exportar SQLite:**
```bash
SHC134DatabaseProjectManagerCpp.exe exportar --motor sqlite --dbname "mi_base_de_datos.sqlite" --out "respaldo.sql"
```

## 📄 Licencia

Este proyecto está bajo la Licencia MIT. Ver el archivo `LICENSE` para más detalles.

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor, lee las guías de contribución antes de enviar un pull request.

## 🐛 Reportar Problemas

Si encuentras algún problema, por favor crea un issue en el repositorio de GitHub con la información detallada del error.

## 📞 Soporte

Para soporte técnico o consultas adicionales, puedes:
- Crear un issue en GitHub
- Revisar la documentación incluida
- Consultar los ejemplos de uso proporcionados

---

**Desarrollado con ❤️ para facilitar la gestión de bases de datos**