import { Controller, Get, Post, Body, Patch, Param, Delete, UseGuards, ParseIntPipe, NotFoundException } from '@nestjs/common';
import { {{ tabla.nombre_clase }}Service } from './{{ tabla.nombre_archivo }}.service';
import { Crear{{ tabla.nombre_clase }}Dto } from './dto/crear-{{ tabla.nombre_archivo }}.dto';
import { Actualizar{{ tabla.nombre_clase }}Dto } from './dto/actualizar-{{ tabla.nombre_archivo }}.dto';
{% if tabla.es_protegida %}
import { JwtAuthGuard } from '../autenticacion/guardianes/jwt-auth.guard';
{% endif %}

{% if tabla.es_protegida %}
@UseGuards(JwtAuthGuard)
{% endif %}
@Controller('{{ tabla.nombre_archivo }}')
export class {{ tabla.nombre_clase }}Controller {
  constructor(private readonly {{ tabla.nombre_variable }}Service: {{ tabla.nombre_clase }}Service) {}

  @Post()
  crear(@Body() crearDto: Crear{{ tabla.nombre_clase }}Dto) {
    return this.{{ tabla.nombre_variable }}Service.crear(crearDto);
  }

  @Get()
  obtenerTodos() {
    return this.{{ tabla.nombre_variable }}Service.obtenerTodos();
  }

  @Get(':id')
  async obtenerUnoPorId(@Param('id', ParseIntPipe) id: number) {
    const registro = await this.{{ tabla.nombre_variable }}Service.obtenerUnoPorId(id);
    if(!registro) {
        throw new NotFoundException(`El registro con id ${id} no fue encontrado.`);
    }
    return registro;
  }

  @Patch(':id')
  actualizar(@Param('id', ParseIntPipe) id: number, @Body() actualizarDto: Actualizar{{ tabla.nombre_clase }}Dto) {
    return this.{{ tabla.nombre_variable }}Service.actualizar(id, actualizarDto);
  }

  @Delete(':id')
  eliminar(@Param('id', ParseIntPipe) id: number) {
    return this.{{ tabla.nombre_variable }}Service.eliminar(id);
  }
}