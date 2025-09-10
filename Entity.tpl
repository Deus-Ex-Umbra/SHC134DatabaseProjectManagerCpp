{% for fk in tabla.dependencias_imports %}
import { {{ fk.clase_tabla_referenciada }} } from '../../{{ fk.archivo_tabla_referenciada }}/entidades/{{ fk.archivo_tabla_referenciada }}.entity';
{% endfor %}
import { Entity, Column, PrimaryGeneratedColumn, ManyToOne, JoinColumn } from 'typeorm';
{% if tabla.es_tabla_usuario %}
import * as bcrypt from 'bcrypt';
{% endif %}

@Entity({ name: '{{ tabla.nombre }}' })
export class {{ tabla.nombre_clase }} {
## for col in tabla.columnas
{% if not col.es_fk %}
{% if col.es_pk %}
  @PrimaryGeneratedColumn({ name: '{{ col.nombre }}' })
  {{ col.nombre }}: {{ col.tipo_ts }};
{% else %}
  @Column({ name: '{{ col.nombre }}', type: '{{ col.tipo_db }}', nullable: {% if col.es_nulo %}true{% else %}false{% endif %} })
  {{ col.nombre }}: {{ col.tipo_ts }};
{% endif %}
{% endif %}
## endfor

## for fk in tabla.dependencias_relaciones
  @ManyToOne(() => {{ fk.clase_tabla_referenciada }})
  @JoinColumn({ name: '{{ fk.columna_local }}' })
  {{ fk.variable_tabla_referenciada }}: {{ fk.clase_tabla_referenciada }};
## endfor

{% if tabla.es_tabla_usuario %}
  async validarContrasena(contrasena: string): Promise<boolean> {
    return bcrypt.compare(contrasena, this.{{ tabla.campo_contrasena }});
  }
{% endif %}
}