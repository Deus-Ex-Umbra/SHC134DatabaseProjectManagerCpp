CREATE EXTENSION IF NOT EXISTS pgcrypto;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_cifrado_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud_cifrado();

CREATE OR REPLACE FUNCTION public.{{ tabla }}_aud_cifrado()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {{ lista_columnas_cifradas }},
            "UsuarioAccion", "FechaAccion", "AccionSql"
        ) VALUES (
            {{ lista_valores_cifrados }},
            SESSION_USER, NOW(), 'Insertado'
        );
        RETURN NEW;
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER {{ tabla }}_aud_cifrado_trigger
AFTER INSERT ON public.{{ tabla }}
FOR EACH ROW EXECUTE PROCEDURE public.{{ tabla }}_aud_cifrado();