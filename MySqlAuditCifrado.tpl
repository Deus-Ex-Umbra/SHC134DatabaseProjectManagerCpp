DROP TRIGGER IF EXISTS update_{{ tabla }}_aud_cifrado;
DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud_cifrado;

CREATE TRIGGER update_{{ tabla }}_aud_cifrado
AFTER UPDATE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }}(
        {{ lista_columnas_cifradas }},
        UsuarioAccion, FechaAccion, AccionSql
    ) VALUES (
        {{ lista_valores_cifrados }},
        SUBSTRING_INDEX(CURRENT_USER(),'@',1), NOW(), 'Modificado'
    );
END;

CREATE TRIGGER delete_{{ tabla }}_aud_cifrado
AFTER DELETE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }}(
        {{ lista_columnas_cifradas }},
        UsuarioAccion, FechaAccion, AccionSql
    ) VALUES (
        {{ lista_valores_cifrados }},
        SUBSTRING_INDEX(CURRENT_USER(),'@',1), NOW(), 'Eliminado'
    );
END;