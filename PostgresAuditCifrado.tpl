CREATE EXTENSION IF NOT EXISTS pgcrypto;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud();
CREATE OR REPLACE FUNCTION public.{{ tabla }}_aud_cifrado()
RETURNS TRIGGER AS $$
DECLARE
    clave_cifrado_hex CONSTANT TEXT := '{{ clave_hex }}';
    clave_cifrado_bytea BYTEA;
BEGIN
    clave_cifrado_bytea := DECODE(clave_cifrado_hex, 'hex');

    IF TG_OP = 'UPDATE' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {% for col in columnas %}"{{ col.nombre_cifrado }}"{% if not loop.last %}, {% endif %}{% endfor %}, 
            "UsuarioAccion", "FechaAccion", "AccionSql"
        ) VALUES (
            {% for col in columnas %}PGP_SYM_ENCRYPT(OLD.{{ col.nombre }}::TEXT, clave_cifrado_hex){% if not loop.last %}, {% endif %}{% endfor %},
            SESSION_USER, NOW(), 'Modificado'
        );
        RETURN NEW;
    ELSIF TG_OP = 'DELETE' THEN
        INSERT INTO public.{{ tabla_auditoria }}(
            {% for col in columnas %}"{{ col.nombre_cifrado }}"{% if not loop.last %}, {% endif %}{% endfor %}, 
            "UsuarioAccion", "FechaAccion", "AccionSql"
        ) VALUES (
            {% for col in columnas %}PGP_SYM_ENCRYPT(OLD.{{ col.nombre }}::TEXT, clave_cifrado_hex){% if not loop.last %}, {% endif %}{% endfor %},
            SESSION_USER, NOW(), 'Eliminado'
        );
        RETURN OLD;
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER {{ tabla }}_aud_cifrado_trigger
AFTER UPDATE OR DELETE ON public.{{ tabla }}
FOR EACH ROW EXECUTE PROCEDURE public.{{ tabla }}_aud_cifrado();