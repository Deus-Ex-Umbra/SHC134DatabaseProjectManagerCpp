DROP FUNCTION IF EXISTS encrypt_val;
DELIMITER $$
CREATE FUNCTION encrypt_val(data_to_encrypt TEXT)
RETURNS TEXT CHARSET utf8mb4
DETERMINISTIC
BEGIN
    DECLARE iv VARBINARY(16);
    DECLARE encrypted_data TEXT;
    IF data_to_encrypt IS NULL OR data_to_encrypt = 'NULL' THEN
        RETURN data_to_encrypt;
    END IF;
    SET iv = RANDOM_BYTES(16);
    SET encrypted_data = HEX(AES_ENCRYPT(data_to_encrypt, UNHEX('{{ clave_hex }}'), iv));
    RETURN CONCAT(HEX(iv), encrypted_data);
END$$
DELIMITER ;

DROP TRIGGER IF EXISTS insert_{{ tabla }}_aud_cifrado;
DROP TRIGGER IF EXISTS update_{{ tabla }}_aud_cifrado;
DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud_cifrado;

DELIMITER $$

CREATE TRIGGER insert_{{ tabla }}_aud_cifrado
AFTER INSERT ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}({{ lista_columnas_cifradas }}) VALUES ({{ valores_insert_new }});
END$$

CREATE TRIGGER update_{{ tabla }}_aud_cifrado
AFTER UPDATE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}({{ lista_columnas_cifradas }}) VALUES ({{ valores_update_old }});
END$$

CREATE TRIGGER delete_{{ tabla }}_aud_cifrado
AFTER DELETE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO {{ tabla_auditoria }}({{ lista_columnas_cifradas }}) VALUES ({{ valores_delete_old }});
END$$

DELIMITER ;