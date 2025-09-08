import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { {{ tabla.nombre_clase }}Service } from './{{ tabla.nombre_archivo }}.service';
import { {{ tabla.nombre_clase }}Controller } from './{{ tabla.nombre_archivo }}.controller';
import { {{ tabla.nombre_clase }} } from './entidades/{{ tabla.nombre_archivo }}.entity';

@Module({
  imports: [TypeOrmModule.forFeature([{{ tabla.nombre_clase }}])],
  controllers: [{{ tabla.nombre_clase }}Controller],
  providers: [{{ tabla.nombre_clase }}Service],
{% if tabla.es_tabla_usuario %}
  exports: [{{ tabla.nombre_clase }}Service],
{% endif %}
})
export class {{ tabla.nombre_clase }}Module {}