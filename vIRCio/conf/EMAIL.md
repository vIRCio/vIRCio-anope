# Email dos Services - preparação e registro de validação

Data: 12/10/2026. Branch: `vIRCio-2.1`.

**Estado: PREPARADO, NÃO HABILITADO E NÃO HOMOLOGADO.** Nenhum teste de
cadastro/confirmação/recuperação por email foi executado nesta rodada.
Não houve restart, migração, criação de contas ou envio de mensagens reais.

## Decisão de transporte e arquivos

- Transporte escolhido: Postfix local existente, interface sendmail-compatible.
- Comando nativo preparado: `/usr/sbin/sendmail -i -t -f services@vircio.net`.
- Remetente preparado: `vIRCio Services <services@vircio.net>`.
- Entrega externa pretendida: direta, sem relay externo. Os cabeçalhos podem
  expor o IP do MTA; não há promessa de ocultação.
- `mail.conf.example` deve ser instalado como `~/anope/conf/mail.conf`.
- `email/pt_BR/` contém versões texto/HTML para cadastro, recuperação,
  mudança de email e recados. O fallback preparado é PT-BR, coerente com
  `defaultlanguage = "pt_BR.UTF-8"`; outros idiomas não foram homologados.
- `anope.conf.example` inclui `mail.conf`. `usemail = no` permanece na
  preparação, assim como `registration = "code"` e
  `confirmemailchanges = no` no NickServ.
- Runtime e modelos receberam a mesma preparação, sem copiar segredos.

Os templates não possuem imagens remotas, pixels, links de confirmação web,
senhas ou parâmetros de autenticação persistente. HTML usa variáveis `.html`
para impedir inserção de marcação pelos valores. Os assuntos são ASCII.
O template MemoServ é necessário à carga nativa com email habilitado; sua
presença não ativa notificações nem altera preferências de contas existentes.

## Preservação das traduções

Nenhuma branch foi trocada. A branch `v2` do InspIRCd permanece em
`35d73ec3b58765cb471c0b4f983a81fd28a31344`, sem modificações nesta tarefa.
O inventário daquela árvore encontrou locales do IRCd, ajuda e código, não
templates de email Anope. Esses locales não são tratados como catálogos de
mensagens traduzidas nem importados para o Anope.

Os catálogos `language/*.po` do Anope e todos os templates existentes em
`data/email/` e `~/anope/conf/email/` foram preservados. Os novos arquivos
PT-BR são adicionais. Há hashes de 60 arquivos preexistentes no snapshot
privado da rodada para verificar preservação.

## Infraestrutura observada

| Verificação | Resultado | Limite |
|:--|:--|:--|
| Postfix | Serviço ativo, sendmail executável | Não comprova entrega |
| Listener SMTP | Somente loopback IPv4/IPv6 | Não abrir relay público |
| TCP/25 externo | Conexão alcançou MX público de referência | Não enviou email; não prova entregabilidade |
| Identidade do MTA | `myhostname = irc.localdomain` | Precisa identidade DNS adequada |
| Relay | Não configurado | Entrega direta pretendida |
| TLS SMTP de saída | Política não configurada | Não enviar códigos de recuperação sem proteção |
| SPF de `vircio.net` | Dois registros `v=spf1` | Política inválida; precisa um único registro |
| DKIM local | OpenDKIM e milters não configurados | Gerar chave privada restrita e publicar seletor |
| DMARC | Nenhum registro encontrado | Definir política coerente com SPF/DKIM, sem prometer conformidade |
| Privilégio administrativo | `sudo -n -l` exige senha | Agente não alterou `/etc/postfix` nem instalou pacotes |
| Caixas técnicas | Ainda não definidas/provisionadas | Necessárias para recebimento, consumo de códigos e falhas controladas |

O domínio já publica MX da Zoho. Isso não fornece credenciais de relay nem
prova existência/controle da caixa `services@vircio.net`. Preservar os MX;
não os substituir pelo servidor IRC apenas para enviar mensagens.

## Pendências administrativas antes da ativação

Executar a parte administrativa em sessão autorizada com root, sem fornecer
senha pelo chat. Revisar consumidores atuais do Postfix antes de mudar
políticas globais; fazer backup privado e verificar `postfix check`.

1. Configurar identidade de saída, por exemplo `services.vircio.net`, que já
   resolve para o servidor. Validar endereço público efetivamente usado,
   PTR/HELO coerentes e CA do sistema. Não alterar o hostname global do host.
2. Manter listener/rede confiável somente locais e impedir open relay.
3. Exigir TLS validado para mensagens de recuperação externas. Candidatos:
   `smtp_tls_security_level = verify`, `smtp_tls_verify_cert_match = hostname`
   e `smtp_tls_CAfile = /etc/ssl/certs/ca-certificates.crt`. Confirmar
   compatibilidade com os MX destinatários. Sem TLS/certificado válido,
   mensagens devem ser adiadas, não cair para texto claro.
4. Instalar/configurar assinatura DKIM. Garantir assinatura também para
   submissão pelo sendmail, via `non_smtpd_milters`, não só `smtpd_milters`.
   Chave privada fica exclusivamente fora do Git e com acesso restrito.
5. Corrigir SPF para um único registro, preservando remetentes existentes
   autorizados, inclusive Zoho. Não remover registros de terceiros por
   suposição. Publicar chave DKIM e definir DMARC no provedor DNS.
6. Provisionar duas caixas técnicas locais distintas e acessíveis apenas à
   homologação. Um domínio reservado `.invalid`, explicitamente tratado como
   local pelo Postfix, evita encaminhar fixtures a terceiros. Entrega nessas
   caixas só comprova entrega local, não recebimento externo.
7. Definir caixa externa controlada para teste de recebimento, pasta de spam,
   cabeçalhos SPF/DKIM/DMARC e retorno de falhas. Confirmar o destino dos
   bounces do envelope `services@vircio.net` sem presumir que essa caixa existe.

Nenhuma dessas operações administrativas foi executada nesta rodada.

## Ativação nativa e restart posterior

Após validar as dependências, aplicar no runtime e nos modelos:

| Arquivo | Ajuste |
|:--|:--|
| `mail.conf` / `mail.conf.example` | `usemail = yes` |
| `nickserv.conf` / `nickserv.conf.example` | `registration = "mail"` no módulo `ns_register` |
| `nickserv.conf` / `nickserv.conf.example` | `confirmemailchanges = yes` no módulo `nickserv` |

Preservar `maxemails = 1`, `remove_aliases = no`, atrasos e prazos atuais.
Não reduzir proteções globais apenas para acelerar testes. Os módulos
`ns_confirm`, `ns_register`, `ns_email` e `ns_resetpass` e as rotas de comandos
já estão declarados; comprovar carga real após ativação. Não implementar RPC
ou alterar módulos/core nesta etapa.

O usuário autorizou restart dos Services após reconfiguração. Executá-lo
somente após configuração válida; `RESTART vIRCio` exige o nome exato da rede.
Confirmar novo PID, link em 7008, API restrita a loopback, módulos e logs.
Não reiniciar o InspIRCd. Preparar recuperação operacional antes do restart.

## Matriz de homologação ainda não executada

Usar apenas contas e caixas técnicas identificadas como fixtures. Nunca usar
contas legadas ou credenciais humanas da equipe para testes destrutivos.
Registrar códigos/senhas somente onde indispensável e com acesso privado;
não incluí-los no relatório, histórico de shell ou Git. Limpeza deve usar
Services exclusivamente sobre fixtures criadas pela homologação, sem editar
o JSON autoritativo. Não presumir que a permissão de teste autoriza recriar
ou excluir contas legadas.

| Cenário | Resultado que deve ser observado | Estado |
|:--|:--|:--|
| REGISTER nativo | Conta pendente e email recebido | PENDENTE |
| CONFIRM REGISTER | Confirmar com código recebido; rejeitar código inválido/replay | PENDENTE |
| RESETPASS | Receber código, confirmar, definir nova senha e testar identificação | PENDENTE |
| Senha anterior | Após mudança, senha anterior não autentica | PENDENTE |
| SET EMAIL | Receber no novo endereço; antigo permanece canônico até confirmar | PENDENTE |
| CONFIRM EMAIL | Novo endereço canônico; código inválido/replay não altera dados | PENDENTE |
| Endereço repetido | Recusar em REGISTER e SET EMAIL, respeitando atrasos | PENDENTE |
| SASET/RPC/admin | Verificar unicidade sem presumir cobertura de `maxemails` | PENDENTE |
| Falha antes de enfileirar | Erro de submissão observável; nenhuma confirmação inferida | PENDENTE |
| Falha após enfileirar | Deferred/bounce correlacionado; não confundir aceite com recebimento | PENDENTE |
| Reenvio | Observar atrasos e comportamento dos códigos; não supor invalidação segura | PENDENTE |
| Entrega externa | Recebimento comprovado e autenticação dos cabeçalhos | PENDENTE |

RESETPASS nativo confirma posse, identifica a sessão IRC e permite escolher
senha com SET PASSWORD. Não envia senha temporária. O código inspecionado
também pode confirmar conta pendente nesse fluxo; incluir esse caso no teste.
O código de recuperação não é evidência de mudança automática de senha.

Uma falha do comando sendmail e uma rejeição SMTP posterior são situações
distintas. O módulo nativo dispara envio em thread e pode responder antes do
resultado de transporte. Seu log "Successfully delivered" quando o processo
retorna zero não comprova recebimento final. Registrar fila, DSN e caixa de
destino sem publicar o conteúdo ou credenciais.

O aviso protegido ao endereço anterior não foi demonstrado no `ns_email`
nativo e não está implementado por este template. Alterações administrativas
têm caminhos diferentes. Habilitar confirmação não homologa esses contratos.
Cadastro web/JSON-RPC de conta nova, incluindo a pendência Anope #615, continua
fora da homologação do REGISTER IRC.

## Inventário agregado de emails

Leitura restrita do snapshot salvo `~/anope/data/anope.json`: **2 contas,
2 aliases, 2 emails distintos, nenhum email vazio e nenhum grupo duplicado**
pela comparação ASCII sem distinção de caixa, preservando pontos e `+`.
Nenhum endereço, nome, identificador ou hash de senha foi publicado.

Isso não é auditoria dos 165 registros históricos mencionados pelo usuário.
O arquivo daquele acervo não foi identificado neste escopo. Nenhuma conta
foi migrada, fundida, recriada, renomeada ou excluída.

## Registro final desta preparação

| Verificação executada | Resultado |
|:--|:--|
| Variáveis das quatro finalidades | Permitidas pelo código nativo inspecionado |
| HTML | Valores escapados; sem conteúdo ativo ou recursos remotos |
| MIME sintético | Duas partes texto/HTML UTF-8 para cada finalidade |
| Runtime/modelos | Configuração de mail e oito templates idênticos |
| Permissões | Arquivos novos no runtime 0600; permissões anteriores preservadas |
| Traduções/templates preexistentes | 60 hashes inalterados |
| InspIRCd `v2` | Referência inalterada; nenhum arquivo modificado nesta tarefa |
| Diff de arquivos rastreados | `git diff --check` passou |
| Processos | PIDs preservados; nenhum reload/restart nesta preparação |

Essas verificações não executam o parser nativo nem comprovam os fluxos da
matriz acima. Não houve submissão nem recebimento de email real.
Atualizar este documento com resultados reais após executar a homologação,
mantendo casos bloqueados/pendentes separados de aprovados.
