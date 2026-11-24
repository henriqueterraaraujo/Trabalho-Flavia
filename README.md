# Sistema de Controle de Estoque (C)

Projeto em C com struct, funções, arquivo binário e exportação para TXT.

## Como compilar e executar

Você precisa ter o compilador **GCC** instalado. Para conferir, abra o terminal e digite:

```
gcc --version
```

Se aparecer um número de versão, está pronto. Se der erro, veja "Instalar o GCC" mais abaixo.

### Passo 1: abrir o terminal na pasta do projeto
A pasta onde está o arquivo `estoque.c`.
- **Windows:** abra a pasta, clique na barra de endereço, digite `cmd` e aperte Enter.
- **Linux/macOS:** abra o terminal e use `cd caminho/da/pasta`.

### Passo 2: compilar
```
gcc -std=c99 -o estoque estoque.c
```
Isso cria o programa `estoque` (no Windows, `estoque.exe`).

### Passo 3: executar
- **Windows:** `estoque.exe`
- **Linux/macOS:** `./estoque`

O menu do sistema vai aparecer na tela.

## Instalar o GCC (só se o passo de conferir deu erro)

- **Windows:** instale o Code::Blocks versão **"mingw-setup"** (https://www.codeblocks.org/downloads/binaries/). Ele já inclui o GCC. Depois, reabra o terminal.
- **Ubuntu/Debian:** `sudo apt install build-essential`
- **macOS:** `xcode-select --install`

## Arquivos
- `estoque.c`: código-fonte
- `estoque.dat`: dados salvos (criado ao cadastrar o primeiro produto)
- `estoque.txt`: relatório (criado ao exportar)

## Equipe
- Henrique Terra de Araujo
