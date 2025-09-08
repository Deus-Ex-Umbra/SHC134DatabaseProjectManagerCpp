import { IsString, IsNumber, IsOptional, IsDate, IsBoolean, IsDefined } from 'class-validator';

export class Crear{{ tabla.nombre_clase }}Dto {
## for col in tabla.columnas
{% if not col.es_pk %}
{% if col.es_nulo %}
    @IsOptional()
{% else %}
    @IsDefined()
{% endif %}
    {{ col.decorador_tipo }}
    readonly {{ col.nombre_camel_case }}: {{ col.tipo_ts }};
{% endif %}
## endfor
}