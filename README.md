# Sistema de Controle de Estoque (C)

Projeto acadêmico em C com struct, modularização, arquivo binário e exportação para TXT.

## Funcionalidades
- Cadastrar produto (código automático)
- Consultar por código
- Listar, alterar e movimentar estoque (entrada/saída)
- Excluir produto (exclusão lógica, campo `status`)
- Exportar para `estoque.txt`
- Contagem de produtos ativos exibida no menu

## Compilar e executar
```
gcc -std=c99 -Wall -Wextra -o estoque estoque.c
./estoque
```

## Arquivos
- `estoque.c`: código-fonte
- `estoque.dat`: banco binário (gerado ao cadastrar)
- `estoque.txt`: relatório (gerado ao exportar)

## Equipe
- Nome 1
- Nome 2
- Nome 3
- Nome 4
