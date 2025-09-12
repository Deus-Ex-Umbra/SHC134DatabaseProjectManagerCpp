DROP TABLE IF EXISTS public.aud_{{ tabla }};
CREATE TABLE public.aud_{{ tabla }} ({{ definicion_columnas }});
ALTER TABLE public.aud_{{ tabla }} ADD COLUMN "UsuarioAccion" TEXT, ADD COLUMN "FechaAccion" TEXT, ADD COLUMN "AccionSql" TEXT;
CREATE OR REPLACE FUNCTION public.{{ tabla }}_aud() RETURNS TRIGGER AS $$ BEGIN IF TG_OP = 'INSERT' THEN INSERT INTO public.aud_{{ tabla }} SELECT NEW.*, SESSION_USER, NOW()::TEXT, 'Insertado';
RETURN NEW; ELSIF TG_OP = 'UPDATE' THEN INSERT INTO public.aud_{{ tabla }} SELECT OLD.*, SESSION_USER, NOW()::TEXT, 'Modificado'; RETURN NEW;
ELSIF TG_OP = 'DELETE' THEN INSERT INTO public.aud_{{ tabla }} SELECT OLD.*, SESSION_USER, NOW()::TEXT, 'Eliminado'; RETURN OLD; END IF;
RETURN NULL; END; $$ LANGUAGE plpgsql;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_trigger ON public.{{ tabla }};
CREATE TRIGGER {{ tabla }}_aud_trigger AFTER INSERT OR UPDATE OR DELETE ON public.{{ tabla }} FOR EACH ROW EXECUTE PROCEDURE public.{{ tabla }}_aud();