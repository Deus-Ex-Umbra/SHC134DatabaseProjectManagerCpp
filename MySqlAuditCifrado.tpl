DROP TRIGGER IF EXISTS update_{{ tabla }}_aud;
DROP TRIGGER IF EXISTS delete_{{ tabla }}_aud;

CREATE TRIGGER update_{{ tabla }}_aud_cifrado
AFTER UPDATE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }}(
        {% for col in columnas %}`{{ col.nombre_cifrado }}`{% if not loop.last %}, {% endif %}{% endfor %}, 
        UsuarioAccion, FechaAccion, AccionSql
    ) VALUES (
        {% for col in columnas %}AES_ENCRYPT(OLD.{{ col.nombre }}, UNHEX('{{ clave_hex }}')){% if not loop.last %}, {% endif %}{% endfor %},
        SUBSTRING_INDEX(CURRENT_USER(),'@',1), NOW(), 'Modificado'
    );
END;

CREATE TRIGGER delete_{{ tabla }}_aud_cifrado
AFTER DELETE ON {{ tabla }}
FOR EACH ROW
BEGIN
    INSERT INTO aud_{{ tabla }}(
        {% for col in columnas %}`{{ col.nombre_cifrado }}`{% if not loop.last %}, {% endif %}{% endfor %}, 
        UsuarioAccion, FechaAccion, AccionSql
    ) VALUES (
        {% for col in columnas %}AES_ENCRYPT(OLD.{{ col.nombre }}, UNHEX('{{ clave_hex }}')){% if not loop.last %}, {% endif %}{% endfor %},
        SUBSTRING_INDEX(CURRENT_USER(),'@',1), NOW(), 'Eliminado'
    );
END;