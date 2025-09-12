import { TypeOrmModuleOptions } from '@nestjs/typeorm';
import { join } from 'path';

export const typeOrmConfig: TypeOrmModuleOptions = {
  type: process.env.DB_TYPE as any || 'postgres',
  host: process.env.DB_HOST || 'localhost',
  port: parseInt(process.env.DB_PORT) || 5432,
  username: process.env.DB_USERNAME || 'root',
  password: process.env.DB_PASSWORD,
  database: process.env.DB_DATABASE || 'nest_db',
  entities: [join(__dirname, '**', '*.entity.{ts,js}')],
  synchronize: false,
  logging: process.env.NODE_ENV === 'development',
  ...(process.env.DB_TYPE === 'mssql' && {
    options: {
      encrypt: true,
      trustServerCertificate: true,
    }
  })
};