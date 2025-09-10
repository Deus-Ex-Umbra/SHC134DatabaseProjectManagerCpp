DROP TRIGGER IF EXISTS insert_{{ tabla }}_aud_cifrado;

CREATE TRIGGER insert_{{ tabla }}_aud_cifrado
AFTER INSERT ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}(
        {{ lista_columnas_cifradas }},
        UsuarioAccion, FechaAccion, AccionSql
    ) VALUES (
        {{ lista_valores_cifrados }},
        SUBSTRING_INDEX(CURRENT_USER(),'@',1), NOW(), 'Insertado'
    );
END;