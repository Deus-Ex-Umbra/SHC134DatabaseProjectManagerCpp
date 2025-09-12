IF OBJECT_ID(N'dbo.fcampos', N'FN') IS NOT NULL
    DROP FUNCTION dbo.fcampos;
GO

CREATE FUNCTION dbo.fcampos (@tabla NVARCHAR(250)) 
RETURNS NVARCHAR(MAX) AS 
BEGIN 
    DECLARE @valores NVARCHAR(MAX) = N'';
    DECLARE @Nombre NVARCHAR(250); 
    DECLARE cur CURSOR FOR SELECT C.name FROM sys.columns C WHERE C.object_id = OBJECT_ID(@tabla) ORDER BY C.column_id;
    OPEN cur; 
    FETCH NEXT FROM cur INTO @Nombre; 
    WHILE (@@FETCH_STATUS = 0) 
    BEGIN 
        SET @valores = @valores + QUOTENAME(@Nombre) + N' NVARCHAR(MAX), ';
        FETCH NEXT FROM cur INTO @Nombre; 
    END;
    CLOSE cur; 
    DEALLOCATE cur; 
    SET @valores = @valores + N'[UsuarioAccion] NVARCHAR(MAX), [FechaAccion] NVARCHAR(MAX), [AccionSql] NVARCHAR(MAX)';
    RETURN @valores; 
END;
GO

IF OBJECT_ID(N'dbo.aud_trigger', N'P') IS NOT NULL 
    DROP PROCEDURE dbo.aud_trigger;
GO

CREATE PROC dbo.aud_trigger @tabla NVARCHAR(250) AS 
BEGIN 
    DECLARE @campos NVARCHAR(MAX) = dbo.fcampos(@tabla);
    DECLARE @createTable NVARCHAR(MAX);
    DECLARE @trigger NVARCHAR(MAX);
    DECLARE @audTabla NVARCHAR(250) = N'Aud' + @tabla;
    DECLARE @triggerName NVARCHAR(250) = N'Trg' + @tabla + 'Aud';

    IF OBJECT_ID(@audTabla, N'U') IS NOT NULL
        EXEC('DROP TABLE ' + @audTabla);
    
    IF OBJECT_ID(@triggerName, N'TR') IS NOT NULL
        EXEC('DROP TRIGGER ' + @triggerName);

    SET @createTable = N'CREATE TABLE ' + @audTabla + N' (' + @campos + N')';
    EXEC sp_executesql @createTable;

    SET @trigger = N'CREATE TRIGGER ' + @triggerName + N' ON dbo.' + @tabla + N' AFTER INSERT, UPDATE, DELETE AS 
    BEGIN 
        IF EXISTS(SELECT * FROM inserted) AND NOT EXISTS(SELECT * FROM deleted) 
        BEGIN 
            INSERT INTO dbo.' + @audTabla + N' SELECT *, SUSER_NAME(), GETDATE(), N''Insertado'' FROM inserted;
        END 
        ELSE IF EXISTS(SELECT * FROM inserted) AND EXISTS(SELECT * FROM deleted) 
        BEGIN 
            INSERT INTO dbo.' + @audTabla + N' SELECT *, SUSER_NAME(), GETDATE(), N''Modificado'' FROM deleted;
        END 
        ELSE IF EXISTS(SELECT * FROM deleted) 
        BEGIN 
            INSERT INTO dbo.' + @audTabla + N' SELECT *, SUSER_NAME(), GETDATE(), N''Eliminado'' FROM deleted; 
        END; 
    END;';
    EXEC sp_executesql @trigger;
END;
GO