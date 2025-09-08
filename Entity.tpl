import { Entity, Column, PrimaryGeneratedColumn } from 'typeorm';
{% if tabla.es_tabla_usuario %}
import * as bcrypt from 'bcrypt';
{% endif %}

@Entity({ name: '{{ tabla.nombre }}' })
export class {{ tabla.nombre_clase }} {
## for col in tabla.columnas
{% if col.es_pk %}
  @PrimaryGeneratedColumn({ name: '{{ col.nombre }}' })
  {{ col.nombre_camel_case }}: {{ col.tipo_ts }};
{% else %}
  @Column({ name: '{{ col.nombre }}', type: '{{ col.tipo_db }}', nullable: {% if col.es_nulo %}true{% else %}false{% endif %} })
  {{ col.nombre_camel_case }}: {{ col.tipo_ts }};
{% endif %}
## endfor

{% if tabla.es_tabla_usuario %}
  async validarContrasena(contrasena: string): Promise<boolean> {
    return bcrypt.compare(contrasena, this.{{ tabla.campo_contrasena }});
  }
{% endif %}
}