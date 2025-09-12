CREATE EXTENSION IF NOT EXISTS pgcrypto;

DROP TRIGGER IF EXISTS {{ tabla }}_aud_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud() CASCADE;
DROP TRIGGER IF EXISTS {{ tabla }}_aud_cifrado_trigger ON public.{{ tabla }};
DROP FUNCTION IF EXISTS public.{{ tabla }}_aud_cifrado() CASCADE;
DROP FUNCTION IF EXISTS public.encrypt_val(TEXT) CASCADE;

CREATE OR REPLACE FUNCTION encrypt_val(data_to_encrypt TEXT)
RETURNS TEXT AS $$
DECLARE
    iv BYTEA := gen_random_bytes(16);
    encrypted_data BYTEA;
    key BYTEA := decode('{{ clave_hex }}', 'hex');
BEGIN
    IF data_to_encrypt IS NULL OR data_to_encrypt = 'NULL' THEN
        RETURN data_to_encrypt;
    END IF;

    encrypted_data := encrypt_iv(data_to_encrypt::bytea, key, iv, 'aes-cbc/pad:pkcs');
    RETURN encode(iv || encrypted_data, 'hex');
END;
$$ LANGUAGE plpgsql;

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