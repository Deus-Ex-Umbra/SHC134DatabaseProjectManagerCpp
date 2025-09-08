IF OBJECT_ID('Trg{{ tabla }}Aud', 'TR') IS NOT NULL
    DROP TRIGGER Trg{{ tabla }}Aud;

GO

CREATE TRIGGER Trg{{ tabla }}AudCifrado
ON dbo.{{ tabla }}
AFTER UPDATE, DELETE
AS
BEGIN
    OPEN SYMMETRIC KEY AuditoriaKey DECRYPTION BY CERTIFICATE AuditoriaCert;

    IF EXISTS(SELECT * FROM INSERTED) AND EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.Aud{{ tabla }} (
            {% for col in columnas %}[{{ col.nombre_cifrado }}]{% if not loop.last %}, {% endif %}{% endfor %},
            UsuarioAccion, FechaAccion, AccionSql
        )
        SELECT 
            {% for col in columnas %}ENCRYPTBYKEY(KEY_GUID('AuditoriaKey'), CONVERT(varchar(max), d.{{ col.nombre }})){% if not loop.last %}, {% endif %}{% endfor %},
            SUSER_NAME(), GETDATE(), 'Modificado'
        FROM DELETED d;
    END
    ELSE IF EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.Aud{{ tabla }} (
            {% for col in columnas %}[{{ col.nombre_cifrado }}]{% if not loop.last %}, {% endif %}{% endfor %},
            UsuarioAccion, FechaAccion, AccionSql
        )
        SELECT 
            {% for col in columnas %}ENCRYPTBYKEY(KEY_GUID('AuditoriaKey'), CONVERT(varchar(max), d.{{ col.nombre }})){% if not loop.last %}, {% endif %}{% endfor %},
            SUSER_NAME(), GETDATE(), 'Eliminado'
        FROM DELETED d;
    END;

    CLOSE SYMMETRIC KEY AuditoriaKey;
END;
GO