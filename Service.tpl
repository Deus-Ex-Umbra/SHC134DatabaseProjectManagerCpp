import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { {{ tabla.nombre_clase }} } from './entidades/{{ tabla.nombre_archivo }}.entity';
import { Crear{{ tabla.nombre_clase }}Dto } from './dto/crear-{{ tabla.nombre_archivo }}.dto';
import { Actualizar{{ tabla.nombre_clase }}Dto } from './dto/actualizar-{{ tabla.nombre_archivo }}.dto';
{% if tabla.es_tabla_usuario %}
import * as bcrypt from 'bcrypt';
{% endif %}

@Injectable()
export class {{ tabla.nombre_clase }}Service {
  constructor(
    @InjectRepository({{ tabla.nombre_clase }})
    private readonly {{ tabla.nombre_variable }}Repositorio: Repository<{{ tabla.nombre_clase }}>,
  ) {}

{% if tabla.es_tabla_usuario %}
  async crear(crearDto: Crear{{ tabla.nombre_clase }}Dto): Promise<{{ tabla.nombre_clase }}> {
    const salt = await bcrypt.genSalt();
    const contrasenaHasheada = await bcrypt.hash((crearDto as any)['{{ tabla.campo_contrasena }}'], salt);
    const dtoConHash = { ...crearDto, ['{{ tabla.campo_contrasena }}']: contrasenaHasheada };
    const nuevoUsuario = this.{{ tabla.nombre_variable }}Repositorio.create(dtoConHash);
    return this.{{ tabla.nombre_variable }}Repositorio.save(nuevoUsuario);
  }
{% else %}
  crear(crearDto: Crear{{ tabla.nombre_clase }}Dto) {
    const nuevoRegistro = this.{{ tabla.nombre_variable }}Repositorio.create(crearDto);
    return this.{{ tabla.nombre_variable }}Repositorio.save(nuevoRegistro);
  }
{% endif %}

  obtenerTodos() {
    return this.{{ tabla.nombre_variable }}Repositorio.find();
  }

  async obtenerUnoPorId(id: number) {
    const registro = await this.{{ tabla.nombre_variable }}Repositorio.findOneBy({ ['{{ tabla.clave_primaria.nombre }}']: id } as any);
    if (!registro) {
      throw new NotFoundException(`Registro con id ${id} no encontrado.`);
    }
    return registro;
  }

{% if tabla.es_tabla_usuario %}
  async buscarPorIdentificadorUnico(identificador: string): Promise<{{ tabla.nombre_clase }} | undefined> {
    return this.{{ tabla.nombre_variable }}Repositorio.findOneBy({ ['{{ tabla.campo_email }}']: identificador } as any);
  }
{% endif %}

  async actualizar(id: number, actualizarDto: Actualizar{{ tabla.nombre_clase }}Dto) {
    const registro = await this.obtenerUnoPorId(id);
    this.{{ tabla.nombre_variable }}Repositorio.merge(registro, actualizarDto);
    return this.{{ tabla.nombre_variable }}Repositorio.save(registro);
  }

  async eliminar(id: number) {
    const resultado = await this.{{ tabla.nombre_variable }}Repositorio.delete(id);
    if (resultado.affected === 0) {
      throw new NotFoundException(`Registro con id ${id} no encontrado.`);
    }
    return { mensaje: `Registro con id ${id} eliminado correctamente.` };
  }
}