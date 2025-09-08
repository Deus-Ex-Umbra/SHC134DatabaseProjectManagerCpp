import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ConfigModule } from '@nestjs/config';
import { typeOrmConfig } from './database/typeorm.config';
## for modulo in modulos
import { {{ modulo.nombreClaseModulo }} } from './{{ modulo.nombreCarpeta }}/{{ modulo.nombreArchivo }}.module';
## endfor

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
    }),
    TypeOrmModule.forRoot(typeOrmConfig),
## for modulo in modulos
    {{ modulo.nombreClaseModulo }},
## endfor
  ],
})
export class AppModule {}