import { Controller, Post, Body, UnauthorizedException } from '@nestjs/common';
import { AuthService } from './auth.service';
import { LoginDto } from './dto/login.dto';

@Controller('autenticacion')
export class AuthController {
  constructor(private authService: AuthService) {}

  @Post('login')
  async login(@Body() loginDto: LoginDto) {
    const usuarioValidado = await this.authService.validarUsuario(loginDto.{{ moduloUsuario.campo_email }}, loginDto.{{ moduloUsuario.campo_contrasena }});
    if (!usuarioValidado) {
      throw new UnauthorizedException('Las credenciales proporcionadas son incorrectas.');
    }
    return this.authService.login(usuarioValidado);
  }
}