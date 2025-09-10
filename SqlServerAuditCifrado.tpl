IF OBJECT_ID('Trg{{ tabla }}AudCifrado', 'TR') IS NOT NULL
    DROP TRIGGER Trg{{ tabla }}AudCifrado;
GO

CREATE TRIGGER Trg{{ tabla }}AudCifrado
ON dbo.{{ tabla }}
AFTER INSERT
AS
BEGIN
    OPEN SYMMETRIC KEY AuditoriaKey DECRYPTION BY CERTIFICATE AuditoriaCert;

    INSERT INTO dbo.{{ tabla_auditoria }} (
        {{ lista_columnas_cifradas }},
        UsuarioAccion, FechaAccion, AccionSql
    )
    SELECT
        {{ lista_valores_cifrados }},
        SUSER_NAME(), GETDATE(), 'Insertado'
    FROM INSERTED i;

    CLOSE SYMMETRIC KEY AuditoriaKey;
END;
GO