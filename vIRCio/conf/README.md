# Configuração vIRCio

Este diretório contém os templates versionados da configuração final da Rede
vIRCio para Anope 2.1.27. Eles são a referência de configuração, não uma cópia
da configuração histórica do Anope 2.0.7.

Arquitetura relevante:

- InspIRCd 4+ em `127.0.0.1:7008`, com Anope iniciando o uplink;
- `db_json` como banco autoritativo;
- JSON-RPC em `127.0.0.1:8080` via `httpd`;
- SASL PLAIN e EXTERNAL; e
- módulos `vircio_defaulttimezone`, `vircio_staffwhois` e `vircio_zombie`.

Copie os arquivos para o runtime somente por procedimento operacional
controlado. Os valores de uplink e JSON-RPC vêm do ambiente privado, nunca
deste repositório. Sistemas externos usam JSON-RPC e não editam `anope.json`.

Não versionar passwords, tokens, chaves, certificados privados, credenciais de
banco, secrets de uplink ou dados privados.
