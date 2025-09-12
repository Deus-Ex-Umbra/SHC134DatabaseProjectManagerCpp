DELIMITER $$
DROP TRIGGER IF EXISTS insert_{{ tabla }}_aud$$
CREATE TRIGGER insert_{{ tabla }}_aud AFTER INSERT ON {{ tabla }} FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }} VALUES({{ campos }}, 'Insertado');
END$$

DROP TRIGGER IF EXISTS update_{{ tabla }}_aud$$
CREATE TRIGGER update_{{ tabla }}_aud AFTER UPDATE ON {{ tabla }} FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }} VALUES({{ campos_old }}, 'Modificado');
END$$

DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud$$
CREATE TRIGGER delete_{{ tabla }}_aud AFTER DELETE ON {{ tabla }} FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }} VALUES({{ campos_old }}, 'Eliminado');
END$$
DELIMITER ;