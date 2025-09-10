import { Injectable } from '@nestjs/common';
import { JwtService } from '@nestjs/jwt';
import { {{ moduloUsuario.nombreClaseServicio }} } from '../{{ moduloUsuario.nombreCarpeta }}/{{ moduloUsuario.nombreArchivo }}.service';

@Injectable()
export class AuthService {
  constructor(
    private usuariosService: {{ moduloUsuario.nombreClaseServicio }},
    private jwtService: JwtService,
  ) {}

  async validarUsuario({{ moduloUsuario.campo_email }}: string, {{ moduloUsuario.campo_contrasena }}: string): Promise<any> {
    const usuario = await this.usuariosService.buscarPorIdentificadorUnico({{ moduloUsuario.campo_email }});
    if (usuario && (await (usuario as any).validarContrasena({{ moduloUsuario.campo_contrasena }}))) {
      const { {{ moduloUsuario.campo_contrasena }}: _, ...resultado } = usuario;
      return resultado;
    }
    return null;
  }

  async login(usuario: any) {
    const payload = { identificador: usuario.{{ moduloUsuario.campo_email }}, sub: usuario.{{ moduloUsuario.clave_primaria.nombre }} };
    return {
      token_de_acceso: this.jwtService.sign(payload),
    };
  }
}