DROP TRIGGER IF EXISTS insert_{{ tabla }}_aud;
DROP TRIGGER IF EXISTS update_{{ tabla }}_aud;
DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud;
DROP TRIGGER IF EXISTS insert_{{ tabla }}_aud_cifrado;
DROP TRIGGER IF EXISTS update_{{ tabla }}_aud_cifrado;
DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud_cifrado;

DELIMITER $$

CREATE TRIGGER insert_{{ tabla }}_aud_cifrado
AFTER INSERT ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}(
        {{ lista_columnas_cifradas }}
    ) VALUES (
        {{ valores_insert_new }}
    );
END$$

CREATE TRIGGER update_{{ tabla }}_aud_cifrado
AFTER UPDATE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}(
        {{ lista_columnas_cifradas }}
    ) VALUES (
        {{ valores_update_old }}
    );
END$$

CREATE TRIGGER delete_{{ tabla }}_aud_cifrado
AFTER DELETE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}(
        {{ lista_columnas_cifradas }}
    ) VALUES (
        {{ valores_delete_old }}
    );
END$$

DELIMITER ;