CREATE EXTENSION IF NOT EXISTS pgcrypto;

DROP TRIGGER IF EXISTS {{ tabla }}_aud_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud() CASCADE;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_cifrado_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud_cifrado() CASCADE;

CREATE OR REPLACE FUNCTION public.{{ tabla }}_aud_cifrado()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {{ lista_columnas_cifradas }}
        ) VALUES (
            {{ valores_insert_new }}
        );
        RETURN NEW;
    ELSIF TG_OP = 'UPDATE' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {{ lista_columnas_cifradas }}
        ) VALUES (
            {{ valores_update_old }}
        );
        RETURN NEW;
    ELSIF TG_OP = 'DELETE' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {{ lista_columnas_cifradas }}
        ) VALUES (
            {{ valores_delete_old }}
        );
        RETURN OLD;
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER {{ tabla }}_aud_cifrado_trigger
AFTER INSERT OR UPDATE OR DELETE ON public.{{ tabla }}
FOR EACH ROW EXECUTE PROCEDURE public.{{ tabla }}_aud_cifrado();