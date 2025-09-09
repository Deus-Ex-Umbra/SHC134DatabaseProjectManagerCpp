import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { join } from 'path';
## for modulo in modulos
import { {{ modulo.nombreClaseModulo }} } from './{{ modulo.nombreCarpeta }}/{{ modulo.nombreArchivo }}.module';
## endfor

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
    }),
    TypeOrmModule.forRootAsync({
      imports: [ConfigModule],
      inject: [ConfigService],
      useFactory: (configService: ConfigService) => ({
        type: configService.get<string>('DB_TYPE') as any,
        host: configService.get<string>('DB_HOST'),
        port: parseInt(configService.get<string>('DB_PORT')),
        username: configService.get<string>('DB_USERNAME'),
        password: configService.get<string>('DB_PASSWORD'),
        database: configService.get<string>('DB_DATABASE'),
        entities: [join(__dirname, '**', '*.entity.{ts,js}')],
        synchronize: false,
        logging: configService.get<string>('NODE_ENV') === 'development',
      }),
    }),
## for modulo in modulos
    {{ modulo.nombreClaseModulo }},
## endfor
  ],
})
export class AppModule {}