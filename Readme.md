SHC134 Database Project Manager
SHC134DatabaseProjectManagerCpp.exe es una potente herramienta de línea de comandos diseñada para automatizar tareas complejas de bases de datos, incluyendo la generación de proyectos API, configuración de auditorías, cifrado de datos sensibles y exportación de respaldos.

Requisitos Previos
Ejecutable: Asegúrate de tener el archivo SHC134DatabaseProjectManagerCpp.exe compilado y accesible desde tu terminal.

Drivers de Base de Datos:

Para conectar con MySQL o SQL Server, es necesario tener instalados los correspondientes drivers ODBC en tu sistema Windows.

Para PostgreSQL y SQLite, generalmente no se requieren instalaciones adicionales.

Uso General
La herramienta se invoca desde la terminal siguiendo una estructura simple:

SHC134DatabaseProjectManagerCpp.exe <accion> [opciones]

<accion>: La operación principal a realizar (scaffolding, auditoria, encriptado, exportar).

[opciones]: Banderas o flags que configuran la acción, como los detalles de conexión a la base de datos.

Opciones Comunes de Conexión
Estas opciones se utilizan en la mayoría de las acciones para establecer la conexión con la base de datos.

Opción

Alias

Descripción

Valor por Defecto

--motor



El motor de base de datos a utilizar. (postgres, mysql, sqlserver, sqlite)

postgres

--host



La dirección del servidor de la base de datos.

localhost

--port



El puerto del servidor de la base de datos.

5432

--dbname



El nombre de la base de datos.

nest_db

--user



El nombre de usuario para la conexión.

root

--password



La contraseña del usuario.

root

Nota: Las opciones --host, --port, --user y --password no son necesarias para SQLite.

Acciones Disponibles
🏗️ Scaffolding
Genera una estructura completa de proyecto API con Nest.js a partir del esquema de una base de datos existente, incluyendo autenticación JWT.

Opciones Específicas:

Opción

Alias

Descripción

Requerido

--out

-o

El directorio donde se creará el proyecto.

No

--jwt-secret



Una clave secreta para firmar los tokens JWT.

Sí

Ejemplos:

PostgreSQL:

SHC134DatabaseProjectManagerCpp.exe scaffolding --motor postgres --host localhost --port 5432 --dbname mi_db --user admin --password "pass" --jwt-secret "MI_CLAVE_SECRETA" --out mi-api

MySQL:

SHC134DatabaseProjectManagerCpp.exe scaffolding --motor mysql --host 127.0.0.1 --port 3306 --dbname mi_db --user root --password "pass" --jwt-secret "MI_CLAVE_SECRETA"

SQLite:

SHC134DatabaseProjectManagerCpp.exe scaffolding --motor sqlite --dbname "C:\ruta\a\mi\db.sqlite" --jwt-secret "MI_CLAVE_SECRETA"

📝 Auditoría
Crea tablas de auditoría (tablas espejo) y los triggers necesarios para registrar automáticamente las operaciones UPDATE y DELETE en las tablas seleccionadas.

Opciones Específicas:

Opción

Alias

Descripción

Requerido

--tabla



Audita una sola tabla. Si se omite, audita todas.

No

Ejemplos:

Auditar todas las tablas en una base de datos PostgreSQL:

SHC134DatabaseProjectManagerCpp.exe auditoria --motor postgres --dbname mi_db --user admin --password "pass"

Auditar únicamente la tabla clientes en MySQL:

SHC134DatabaseProjectManagerCpp.exe auditoria --motor mysql --dbname mi_db --user root --password "pass" --tabla clientes

🔒 Encriptado
Gestiona el cifrado de las tablas de auditoría. Puede cifrar los datos y columnas existentes y modificar los triggers para que los nuevos registros también se cifren, o puede ejecutar consultas descifrando los resultados al vuelo.

Opciones Específicas:

Opción

Alias

Descripción

Requerido

--key

-k

La clave de cifrado en formato hexadecimal de 64 caracteres.

Sí

--encrypt-audit-tables



Cifra todas las tablas de auditoría (aud_*).

No

--query

-q

Ejecuta una consulta SQL y muestra los resultados descifrados.

No

Nota: Debes usar --encrypt-audit-tables O --query, pero no ambos a la vez.

Ejemplos:

Cifrar todas las tablas de auditoría existentes:

SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --dbname mi_db --user admin --password "pass" --key "TU_CLAVE_HEXADECIMAL_DE_64_CARACTERES_AQUI" --encrypt-audit-tables

Consultar una tabla de auditoría y ver los datos descifrados:

SHC134DatabaseProjectManagerCpp.exe encriptado --motor postgres --dbname mi_db --user admin --password "pass" --key "TU_CLAVE_HEXADECIMAL_DE_64_CARACTERES_AQUI" --query "SELECT * FROM aud_usuarios LIMIT 10"

📤 Exportar
Realiza un respaldo completo de la base de datos (incluyendo tablas, rutinas y triggers) en un único archivo SQL.

Opciones Específicas:

Opción

Alias

Descripción

Requerido

--out

-o

La ruta del archivo de respaldo.

Sí

Advertencia: La exportación para SQL Server no está soportada y debe realizarse manualmente desde SQL Server Management Studio.

Ejemplos:

Exportar una base de datos PostgreSQL:

SHC134DatabaseProjectManagerCpp.exe exportar --motor postgres --host localhost --dbname mi_db --user admin --password "pass" --out "C:\Respaldos\mi_db_backup.sql"

Exportar una base de datos MySQL:

SHC134DatabaseProjectManagerCpp.exe exportar --motor mysql --user root --password "pass" --dbname mi_db --out "respaldo_mysql.sql"

Exportar una base de datos SQLite:

SHC134DatabaseProjectManagerCpp.exe exportar --motor sqlite --dbname "mi_base_de_datos.sqlite" --out "respaldo.sql"
