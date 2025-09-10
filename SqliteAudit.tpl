DROP TABLE IF EXISTS aud_{{ tabla }};
CREATE TABLE aud_{{ tabla }} ({{ definicion_columnas }}, UsuarioAccion TEXT, FechaAccion TEXT, AccionSql TEXT);
DROP TRIGGER IF EXISTS {{ tabla }}_aud_insert;
CREATE TRIGGER {{ tabla }}_aud_insert AFTER INSERT ON {{ tabla }} FOR EACH ROW BEGIN INSERT INTO aud_{{ tabla }} VALUES({{ lista_columnas_new }}, 'SYSTEM', datetime('now'), 'Insertado'); END;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_update;
CREATE TRIGGER {{ tabla }}_aud_update AFTER UPDATE ON {{ tabla }} FOR EACH ROW BEGIN INSERT INTO aud_{{ tabla }} VALUES({{ lista_columnas_old }}, 'SYSTEM', datetime('now'), 'Modificado'); END;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_delete;
CREATE TRIGGER {{ tabla }}_aud_delete AFTER DELETE ON {{ tabla }} FOR EACH ROW BEGIN INSERT INTO aud_{{ tabla }} VALUES({{ lista_columnas_old }}, 'SYSTEM', datetime('now'), 'Eliminado'); END;