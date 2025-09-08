import { TypeOrmModuleOptions } from '@nestjs/typeorm';
import { join } from 'path';

export const typeOrmConfig: TypeOrmModuleOptions = {
  type: 'postgres',
  host: 'localhost',
  port: 5450,
  username: 'root',
  password: 'root',
  database: 'nest_db',
  entities: [join(__dirname, '..', '**', '*.entity.{ts,js}')],
  synchronize: false, 
};