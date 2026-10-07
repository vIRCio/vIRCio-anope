# Módulos Anope 2.1 da vIRCio

Os fontes compiláveis ficam em `modules/third/`, para que o CMake do Anope os
descubra. Este diretório é o inventário e a documentação operacional.

## `vircio_staffwhois`

- Publica `Services Root`, `Services Admin` ou `Services Oper` como SWHOIS de
  uma conta online que seja um Services Oper válido.
- Usa `IRCD->SendSWhois()` e `IRCD->SendSWhoisDel()` com a tag exclusiva
  `vircio-services-staff`.
- Só atua quando o protocolo anuncia suporte a SWHOIS múltiplo. Assim não
  substitui SWHOIS de outros módulos no fallback legado.
- Sincroniza login/logout, mudanças do modo IRC `+o`/`+r`, `OperServ OPER`, rehash,
  burst e unload. `Helper` não recebe este SWHOIS.
- Requer a extensão InspIRCd `m_swhois_ext`/capability `swhois_ext`.

## `vircio_defaulttimezone`

- Depende do módulo oficial `ns_set_timezone`, que fornece o extensible
  serializável `timezone`.
- Define `America/Sao_Paulo` apenas para uma conta que ainda não tenha
  preferência explícita.
- Intercepta o reset oficial `NS SET TIMEZONE` (e o equivalente SASET sem
  argumento) para gravar `America/Sao_Paulo` imediatamente; não deixa a conta
  temporariamente em UTC nem informa UTC como valor efetivo.
- Não substitui uma seleção feita pelo usuário nem cria serialização própria.
- Trata contas novas, startup após o banco e carregamento/rehash do módulo.

## `vircio_zombie`

- Expõe o comando semântico `OperServ ZOMBIE ADD|DEL <nick-or-uid>`.
- `ZombieServ` oferece somente `HELP` e `ZOMBIE`, para o OperType mínimo
  `Zombie Scanner` sem depender de IRCop.
- `ADD` só atua sobre usuário online ainda não autenticado; `DEL` retira a
  quarentena de um usuário online que a possua.
- A autorização vem do comando configurado `operserv/zombie`: na configuração
  vIRCio ele é concedido a `Services Admin` e herdado por `Services Root`.
  Há também o OperType independente `Zombie Scanner`, com somente esse
  comando e sem heranças ou privilégios. Nenhuma conta é configurada por
  padrão; ZombieGringo deve usar uma conta dedicada, nunca uma identidade
  humana.
- O estado canônico é o usermode `+Z` (`U_SERVICES_ZOMBIE` no Anope), já
  anunciado pelo CAPAB do InspIRCd. O módulo não registra modo paralelo nem
  altera o adapter de protocolo.
- Aplica o modo pela API `User::SetMode()`/`RemoveMode()`, com pseudoclient
  Services como origem; não emite S2S bruto.
- A aplicação de restrições, autojoin em `#vIRCio`, permissão de mensagens e
  retirada automática após autenticação pertencem exclusivamente a
  `m_vircio_zombie` no InspIRCd.
- Este módulo não contém GeoIP, DNSBL, proxy scan, VPN/hosting ou políticas de
  exceção. Essas decisões permanecem no ZombieGringo externo.

### Provisionamento futuro do ZombieGringo

1. Registrar uma conta exclusiva para a máquina, fora do Git, e atribuir-lhe
   `Zombie Scanner` somente após validação administrativa.
2. Preferir SASL EXTERNAL com certificado cliente dedicado: `ns_sasl_external`
   identifica uma conta cujo certificado esteja em `NS CERT`, e o bloco `oper`
   futuro deve restringir o mesmo `certfp`.
3. Configurar `require_oper = no`, `require_login = no` e nenhuma senha no
   bloco futuro: o scanner não deve ser IRCop nem depender de `/OperServ
   LOGIN`; a autenticação de conta SASL e a validação de certificado continuam
   obrigatórias.
4. Adicionar uma restrição `host` específica como defesa adicional quando o
   endereço/ident da máquina for estável. Host não é autenticação suficiente.
5. Não criar o bloco `oper`, senha, token ou fingerprint até a implantação do
   scanner. SASL PLAIN com senha exclusiva em arquivo privado é fallback, não
   configuração versionada.

## Verificação exigida

Para cada atualização, compilar os módulos, validar carregamento com o
InspIRCd 4+ e testar no mínimo a autorização negativa/positiva do comando
Zombie, o ciclo `+Z` → autenticação → `-Z`, e a sincronização de SWHOIS sem
apagar tags de terceiros.
