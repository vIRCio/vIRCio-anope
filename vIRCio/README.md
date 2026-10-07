# vIRCio — Anope

Camada específica da Rede vIRCio sobre o Anope 2.1.27 para InspIRCd 4+.

## Política

O código upstream deve permanecer o mais limpo possível. A ordem obrigatória é:

1. recurso nativo;
2. configuração;
3. módulo oficial;
4. módulo próprio da vIRCio;
5. patch no core, somente quando inevitável.

A branch `2.1` acompanha o upstream. A branch `vIRCio-2.1` contém somente
alterações específicas da rede e parte da baseline fechada `2.1.27`.

## Estrutura

    vIRCio/conf/
        templates versionados da configuração da rede

    vIRCio/modules/
        inventário, contratos e operação dos módulos próprios

    modules/third/
        fontes C++ compiláveis dos módulos próprios

Os templates usam apenas placeholders de ambiente para segredos. Nunca
versionar passwords, tokens, chaves, certificados privados, credenciais de
banco ou bancos de dados.
