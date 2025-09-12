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
    
    DECLARE @pk_col NVARCHAR(128);
    SELECT @pk_col = c.name
    FROM sys.indexes i
    INNER JOIN sys.index_columns ic ON i.object_id = ic.object_id AND i.index_id = ic.index_id
    INNER JOIN sys.columns c ON ic.object_id = c.object_id AND ic.column_id = c.column_id
    WHERE i.is_primary_key = 1 AND i.object_id = OBJECT_ID('dbo.{{ tabla }}');

    IF EXISTS(SELECT * FROM inserted) AND NOT EXISTS(SELECT * FROM deleted)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_insert_new }}
        FROM inserted i
        INNER JOIN dbo.{{ tabla }} t ON i.[@pk_col] = t.[@pk_col];
    END
    ELSE IF EXISTS(SELECT * FROM inserted) AND EXISTS(SELECT * FROM deleted)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_update_old }}
        FROM deleted d
        INNER JOIN dbo.{{ tabla }} t ON d.[@pk_col] = t.[@pk_col];
    END
    ELSE IF EXISTS(SELECT * FROM deleted)
    BEGIN
        INSERT INTO dbo.{{ tabla_auditoria }} ({{ lista_columnas_cifradas }})
        SELECT {{ valores_delete_old }}
        FROM deleted d;
    END
    
    CLOSE SYMMETRIC KEY AuditoriaKey;
END;
GO