IF OBJECT_ID('Trg{{ tabla }}AudCifrado', 'TR') IS NOT NULL
    DROP TRIGGER Trg{{ tabla }}AudCifrado;
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
            {{ lista_columnas_cifradas }},
            UsuarioAccion, FechaAccion, AccionSql
        )
        SELECT
            {{ lista_valores_cifrados }},
            SUSER_NAME(), GETDATE(), 'Modificado'
        FROM DELETED d;
    END
    ELSE IF EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.Aud{{ tabla }} (
            {{ lista_columnas_cifradas }},
            UsuarioAccion, FechaAccion, AccionSql
        )
        SELECT
            {{ lista_valores_cifrados }},
            SUSER_NAME(), GETDATE(), 'Eliminado'
        FROM DELETED d;
    END;

    CLOSE SYMMETRIC KEY AuditoriaKey;
END;
GO