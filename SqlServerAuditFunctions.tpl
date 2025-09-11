IF OBJECT_ID('fcampos', 'FN') IS NOT NULL DROP FUNCTION fcampos;
CREATE FUNCTION fcampos (@tabla VARCHAR(250)) RETURNS VARCHAR(MAX) AS BEGIN DECLARE @valores varchar(MAX) = '';
DECLARE @Nombre varchar(250), @tipo VARCHAR(50), @longitud VARCHAR(50); DECLARE cur CURSOR FOR SELECT C.name, T.name, C.max_length FROM sys.columns C INNER JOIN sys.types T ON T.system_type_id=C.system_type_id WHERE C.object_id = OBJECT_ID(@tabla) ORDER BY C.column_id;
OPEN cur; FETCH NEXT FROM cur INTO @Nombre,@tipo,@longitud; WHILE (@@FETCH_STATUS = 0) BEGIN 
SET @valores += @Nombre + ' VARCHAR(MAX), '; FETCH NEXT FROM cur INTO @Nombre,@tipo,@longitud; END;
CLOSE cur; DEALLOCATE cur; SET @valores += 'UsuarioAccion varchar(30), FechaAccion datetime, AccionSql varchar(30)'; RETURN @valores; END;
IF OBJECT_ID('aud_trigger', 'P') IS NOT NULL DROP PROCEDURE aud_trigger;
CREATE PROC aud_trigger @tabla VARCHAR(250) AS BEGIN DECLARE @campos varchar(MAX) = dbo.fcampos(@tabla), @createTable VARCHAR(MAX), @trigger VARCHAR(MAX);
SET @createTable = 'CREATE TABLE Aud'+@tabla+' (' +@campos+ ')'; SET @trigger = 'CREATE TRIGGER Trg'+@tabla+'Aud ON dbo.'+@tabla+' AFTER INSERT, UPDATE, DELETE AS BEGIN IF EXISTS(SELECT * FROM INSERTED) AND NOT EXISTS(SELECT * FROM DELETED) BEGIN INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Insertado'' FROM INSERTED;
END ELSE IF EXISTS(SELECT * FROM INSERTED) AND EXISTS(SELECT * FROM DELETED) BEGIN INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Modificado'' FROM DELETED;
END ELSE IF EXISTS(SELECT * FROM DELETED) BEGIN INSERT INTO dbo.Aud'+@tabla+' SELECT *, SUSER_NAME(), GETDATE(), ''Eliminado'' FROM DELETED; END; END;';
IF OBJECT_ID('Aud'+@tabla, 'U') IS NOT NULL EXEC('DROP TABLE Aud'+@tabla); IF OBJECT_ID('Trg'+@tabla+'Aud', 'TR') IS NOT NULL EXEC('DROP TRIGGER Trg'+@tabla+'Aud'); EXEC(@createTable); EXEC(@trigger);
END;