IF NOT EXISTS (SELECT * FROM sys.symmetric_keys WHERE name = 'AuditoriaKey')
BEGIN
    CREATE MASTER KEY ENCRYPTION BY PASSWORD = 'DevPasswordComplexEnough#123!';
    CREATE CERTIFICATE AuditoriaCert WITH SUBJECT = 'Auditoria Certificate';
    CREATE SYMMETRIC KEY AuditoriaKey WITH ALGORITHM = AES_256 ENCRYPTION BY CERTIFICATE AuditoriaCert;
END
GO

IF OBJECT_ID('Trg{{ tabla }}AudCifrado', 'TR') IS NOT NULL
    DROP TRIGGER Trg{{ tabla }}AudCifrado;
GO

CREATE TRIGGER Trg{{ tabla }}AudCifrado
ON dbo.{{ tabla }}
AFTER INSERT, UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;
    OPEN SYMMETRIC KEY AuditoriaKey DECRYPTION BY CERTIFICATE AuditoriaCert;
    
    IF EXISTS(SELECT * FROM INSERTED) AND NOT EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_insert_new }} FROM INSERTED i;
    END
    ELSE IF EXISTS(SELECT * FROM INSERTED) AND EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_update_old }} FROM DELETED d;
    END
    ELSE IF EXISTS(SELECT * FROM DELETED)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_delete_old }} FROM DELETED d;
    END
    
    CLOSE SYMMETRIC KEY AuditoriaKey;
END;
GO