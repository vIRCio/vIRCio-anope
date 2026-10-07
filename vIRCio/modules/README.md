# Módulos vIRCio

Este diretório contém o inventário e os contratos operacionais dos módulos
próprios da Rede vIRCio. Os fontes compiláveis ficam em `modules/third/`, onde
o CMake do Anope os descobre.

Antes de criar ou alterar um módulo próprio:

1. verificar se o Anope já possui o recurso nativamente;
2. verificar módulos oficiais existentes;
3. estudar a API real da versão atual;
4. evitar patches no core;
5. documentar integração com InspIRCd e outros componentes.

Módulos históricos do Anope 2.0.7 não são portados mecanicamente.

Qualquer funcionalidade antiga deverá ser reavaliada e, quando ainda necessária,
reescrita usando a API atual do Anope.

Inventário e contratos dos módulos atuais: [anope-2.1.md](anope-2.1.md).
