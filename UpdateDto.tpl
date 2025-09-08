import { PartialType } from '@nestjs/mapped-types';
import { Crear{{ tabla.nombre_clase }}Dto } from './crear-{{ tabla.nombre_archivo }}.dto';

export class Actualizar{{ tabla.nombre_clase }}Dto extends PartialType(Crear{{ tabla.nombre_clase }}Dto) {}