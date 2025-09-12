IF OBJECT_ID(N'dbo.fcampos', N'FN') IS NOT NULL
    DROP FUNCTION dbo.fcampos;
GO

CREATE FUNCTION dbo.fcampos (@tabla NVARCHAR(250))
RETURNS NVARCHAR(MAX) AS
BEGIN
    DECLARE @valores NVARCHAR(MAX) = N'';
    DECLARE @col_name NVARCHAR(128);

    DECLARE cur CURSOR FOR
        SELECT name
        FROM sys.columns
        WHERE object_id = OBJECT_ID(@tabla)
        ORDER BY column_id;

    OPEN cur;
    FETCH NEXT FROM cur INTO @col_name;

    WHILE (@@FETCH_STATUS = 0)
    BEGIN
        SET @valores = @valores + QUOTENAME(@col_name) + N' VARCHAR(MAX), ';
        FETCH NEXT FROM cur INTO @col_name;
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
    SET NOCOUNT ON;
    DECLARE @campos NVARCHAR(MAX) = dbo.fcampos(@tabla);
    DECLARE @createTable NVARCHAR(MAX);
    DECLARE @trigger NVARCHAR(MAX);
    DECLARE @audTabla NVARCHAR(250) = N'aud_' + @tabla;
    DECLARE @triggerName NVARCHAR(250) = N'Trg' + @tabla + 'Aud';

    IF OBJECT_ID(@audTabla, N'U') IS NOT NULL
        EXEC('DROP TABLE ' + @audTabla);
    IF OBJECT_ID(@triggerName, N'TR') IS NOT NULL
        EXEC('DROP TRIGGER ' + @triggerName);

    SET @createTable = N'CREATE TABLE ' + @audTabla + N' (' + @campos + N')';
    EXEC sp_executesql @createTable;

    DECLARE @insert_col_list NVARCHAR(MAX) = N'';
    DECLARE @select_list NVARCHAR(MAX) = N'';
    DECLARE @col_name NVARCHAR(128);
    DECLARE @col_type NVARCHAR(128);

    DECLARE cur CURSOR LOCAL FAST_FORWARD FOR
        SELECT c.name, t.name
        FROM sys.columns c JOIN sys.types t ON c.user_type_id = t.user_type_id
        WHERE c.object_id = OBJECT_ID(@tabla)
        ORDER BY c.column_id;

    OPEN cur;
    FETCH NEXT FROM cur INTO @col_name, @col_type;

    WHILE (@@FETCH_STATUS = 0)
    BEGIN
        SET @insert_col_list = @insert_col_list + QUOTENAME(@col_name) + N',';

        IF @col_type IN (N'text', N'ntext', N'image')
        BEGIN
            SET @select_list = @select_list + N'''[LOB_DATA_NOT_AUDITED]'',';
        END
        ELSE
        BEGIN
            SET @select_list = @select_list + QUOTENAME(@col_name) + N',';
        END

        FETCH NEXT FROM cur INTO @col_name, @col_type;
    END;
    CLOSE cur;
    DEALLOCATE cur;

    SET @insert_col_list = @insert_col_list + N'[UsuarioAccion],[FechaAccion],[AccionSql]';
    
    SET @trigger = N'CREATE TRIGGER ' + @triggerName + N' ON dbo.' + @tabla + N'
    AFTER INSERT, UPDATE, DELETE AS
    BEGIN
        SET NOCOUNT ON;
        IF EXISTS(SELECT 1 FROM inserted) AND NOT EXISTS(SELECT 1 FROM deleted)
        BEGIN
            INSERT INTO dbo.' + @audTabla + N' (' + @insert_col_list + N')
            SELECT ' + @select_list + N' SUSER_NAME(), GETDATE(), N''Insertado'' FROM inserted;
        END
        ELSE IF EXISTS(SELECT 1 FROM inserted) AND EXISTS(SELECT 1 FROM deleted)
        BEGIN
            INSERT INTO dbo.' + @audTabla + N' (' + @insert_col_list + N')
            SELECT ' + @select_list + N' SUSER_NAME(), GETDATE(), N''Modificado'' FROM deleted;
        END
        ELSE IF EXISTS(SELECT 1 FROM deleted)
        BEGIN
            INSERT INTO dbo.' + @audTabla + N' (' + @insert_col_list + N')
            SELECT ' + @select_list + N' SUSER_NAME(), GETDATE(), N''Eliminado'' FROM deleted;
        END;
    END;';

    EXEC sp_executesql @trigger;
END;
GO