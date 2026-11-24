/*
 * ============================================================================
 *  SISTEMA DE CONTROLE DE ESTOQUE DE PRODUTOS
 * ----------------------------------------------------------------------------
 *  Linguagem  : C (padrão C99)
 *  Persistência: arquivo binário "estoque.dat" (registros struct de tamanho fixo)
 *  Exportação : arquivo texto "estoque.txt"
 *
 *  Funcionalidades obrigatórias:
 *    1. Cadastrar produto
 *    2. Consultar produto por código
 *    3. Exportar registros para TXT
 *    4. Excluir produto (exclusão lógica via campo "status")
 *    5. Contabilizar registros ativos (exibido no próprio menu)
 *
 *  Funcionalidades extras: listar produtos, alterar produto, movimentar estoque.
 *
 *  Compilar: gcc -std=c99 -Wall -Wextra -o estoque estoque.c
 *  Executar: ./estoque        (Windows: estoque.exe)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------ CONSTANTES ------------------------------- */
#define ARQ_BINARIO  "estoque.dat"
#define ARQ_TEXTO    "estoque.txt"

#define TAM_NOME      50
#define TAM_CATEGORIA 30

#define STATUS_ATIVO    'A'   /* registro válido                       */
#define STATUS_EXCLUIDO 'E'   /* registro excluído logicamente         */

/* ------------------------------ ESTRUTURA -------------------------------- */
typedef struct {
    int    codigo;                    /* identificador único (chave)       */
    char   nome[TAM_NOME];
    char   categoria[TAM_CATEGORIA];
    int    quantidade;                /* unidades em estoque               */
    float  preco;                     /* preço unitário (R$)               */
    char   status;                    /* 'A' = ativo, 'E' = excluído       */
} Produto;

/* ------------------- PROTÓTIPOS (organizados por módulo) ----------------- */
/* Utilitários de entrada */
void limparBuffer(void);
void lerTexto(const char *mensagem, char *destino, int tamanho);
int  lerInteiro(const char *mensagem, int minimo);
float lerReal(const char *mensagem, float minimo);
void pausar(void);

/* Acesso ao arquivo binário */
int  proximoCodigo(void);
int  buscarPosicao(int codigo, Produto *encontrado);
int  gravarNovo(const Produto *p);
int  regravarNaPosicao(int posicao, const Produto *p);
int  contarAtivos(void);

/* Exibição */
void exibirProduto(const Produto *p);
void cabecalhoTabela(void);
void linhaTabela(const Produto *p);

/* Funcionalidades do menu */
void cadastrarProduto(void);
void consultarProduto(void);
void listarProdutos(void);
void alterarProduto(void);
void movimentarEstoque(void);
void excluirProduto(void);
void exportarTxt(void);
int  menu(void);

/* =========================================================================
 *                         UTILITÁRIOS DE ENTRADA
 * ========================================================================= */

void limparBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* descarta */ }
}

/* Lê uma linha de texto com segurança (fgets) e remove o '\n'. */
void lerTexto(const char *mensagem, char *destino, int tamanho) {
    do {
        printf("%s", mensagem);
        if (fgets(destino, tamanho, stdin) == NULL) {
            destino[0] = '\0';
            return;
        }
        size_t len = strlen(destino);
        if (len > 0 && destino[len - 1] == '\n') {
            destino[len - 1] = '\0';
        } else {
            limparBuffer();            /* linha maior que o buffer */
        }
        if (destino[0] == '\0')
            printf("  [!] O campo nao pode ficar vazio.\n");
    } while (destino[0] == '\0');
}

/* Lê um inteiro >= minimo, repetindo até a entrada ser válida. */
int lerInteiro(const char *mensagem, int minimo) {
    char linha[32];
    char *fim;
    long valor;
    for (;;) {
        printf("%s", mensagem);
        if (fgets(linha, sizeof(linha), stdin) == NULL) return minimo;
        if (strchr(linha, '\n') == NULL) limparBuffer();
        valor = strtol(linha, &fim, 10);
        if (fim != linha && (*fim == '\n' || *fim == '\0') && valor >= minimo)
            return (int)valor;
        printf("  [!] Valor invalido (minimo permitido: %d).\n", minimo);
    }
}

/* Lê um número real >= minimo (aceita vírgula ou ponto como separador). */
float lerReal(const char *mensagem, float minimo) {
    char linha[32];
    char *fim;
    float valor;
    for (;;) {
        printf("%s", mensagem);
        if (fgets(linha, sizeof(linha), stdin) == NULL) return minimo;
        if (strchr(linha, '\n') == NULL) limparBuffer();
        for (char *p = linha; *p; p++) if (*p == ',') *p = '.';
        valor = strtof(linha, &fim);
        if (fim != linha && (*fim == '\n' || *fim == '\0') && valor >= minimo)
            return valor;
        printf("  [!] Valor invalido (minimo permitido: %.2f).\n", minimo);
    }
}

void pausar(void) {
    printf("\nPressione ENTER para continuar...");
    limparBuffer();
}

/* =========================================================================
 *                       ACESSO AO ARQUIVO BINÁRIO
 * ========================================================================= */

/* Gera o próximo código: maior código já usado + 1 (inclui excluídos,
 * para que um código nunca seja reaproveitado). */
int proximoCodigo(void) {
    FILE *f = fopen(ARQ_BINARIO, "rb");
    Produto p;
    int maior = 0;
    if (f == NULL) return 1;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
        if (p.codigo > maior) maior = p.codigo;
    fclose(f);
    return maior + 1;
}

/* Procura um produto ATIVO pelo código.
 * Retorna a posição (índice do registro no arquivo) ou -1 se não achar.
 * Se "encontrado" não for NULL, recebe uma cópia do registro. */
int buscarPosicao(int codigo, Produto *encontrado) {
    FILE *f = fopen(ARQ_BINARIO, "rb");
    Produto p;
    int pos = 0;
    if (f == NULL) return -1;
    while (fread(&p, sizeof(Produto), 1, f) == 1) {
        if (p.codigo == codigo && p.status == STATUS_ATIVO) {
            if (encontrado) *encontrado = p;
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

/* Acrescenta um registro ao final do arquivo. Retorna 1 se ok, 0 se erro. */
int gravarNovo(const Produto *p) {
    FILE *f = fopen(ARQ_BINARIO, "ab");
    int ok;
    if (f == NULL) return 0;
    ok = (fwrite(p, sizeof(Produto), 1, f) == 1);
    fclose(f);
    return ok;
}

/* Sobrescreve o registro que está na posição indicada. */
int regravarNaPosicao(int posicao, const Produto *p) {
    FILE *f = fopen(ARQ_BINARIO, "r+b");
    int ok;
    if (f == NULL) return 0;
    fseek(f, (long)posicao * (long)sizeof(Produto), SEEK_SET);
    ok = (fwrite(p, sizeof(Produto), 1, f) == 1);
    fclose(f);
    return ok;
}

/* Conta quantos registros com status 'A' existem no arquivo. */
int contarAtivos(void) {
    FILE *f = fopen(ARQ_BINARIO, "rb");
    Produto p;
    int total = 0;
    if (f == NULL) return 0;
    while (fread(&p, sizeof(Produto), 1, f) == 1)
        if (p.status == STATUS_ATIVO) total++;
    fclose(f);
    return total;
}

/* =========================================================================
 *                              EXIBIÇÃO
 * ========================================================================= */

void exibirProduto(const Produto *p) {
    printf("\n  Codigo    : %d\n",      p->codigo);
    printf("  Nome      : %s\n",        p->nome);
    printf("  Categoria : %s\n",        p->categoria);
    printf("  Quantidade: %d un.\n",    p->quantidade);
    printf("  Preco     : R$ %.2f\n",   p->preco);
    printf("  Valor em estoque: R$ %.2f\n", p->preco * p->quantidade);
    printf("  Status    : %s\n",
           p->status == STATUS_ATIVO ? "Ativo" : "Excluido");
}

void cabecalhoTabela(void) {
    printf("\n%-6s %-30s %-18s %6s %12s\n",
           "COD", "NOME", "CATEGORIA", "QTD", "PRECO (R$)");
    printf("---------------------------------------------------------------------------\n");
}

void linhaTabela(const Produto *p) {
    printf("%-6d %-30.30s %-18.18s %6d %12.2f\n",
           p->codigo, p->nome, p->categoria, p->quantidade, p->preco);
}

/* =========================================================================
 *                       FUNCIONALIDADES DO SISTEMA
 * ========================================================================= */

/* 1. CADASTRAR */
void cadastrarProduto(void) {
    Produto p;
    memset(&p, 0, sizeof(Produto));

    printf("\n=== CADASTRAR PRODUTO ===\n");
    p.codigo = proximoCodigo();
    printf("Codigo gerado automaticamente: %d\n", p.codigo);

    lerTexto("Nome       : ", p.nome, TAM_NOME);
    lerTexto("Categoria  : ", p.categoria, TAM_CATEGORIA);
    p.quantidade = lerInteiro("Quantidade : ", 0);
    p.preco      = lerReal   ("Preco (R$) : ", 0.0f);
    p.status     = STATUS_ATIVO;

    if (gravarNovo(&p))
        printf("\n[OK] Produto cadastrado com sucesso! (codigo %d)\n", p.codigo);
    else
        printf("\n[ERRO] Nao foi possivel gravar no arquivo.\n");
    pausar();
}

/* 2. CONSULTAR POR CÓDIGO */
void consultarProduto(void) {
    Produto p;
    int codigo;

    printf("\n=== CONSULTAR PRODUTO ===\n");
    codigo = lerInteiro("Informe o codigo: ", 1);

    if (buscarPosicao(codigo, &p) >= 0)
        exibirProduto(&p);
    else
        printf("\n[!] Produto com codigo %d nao encontrado.\n", codigo);
    pausar();
}

/* Extra: LISTAR todos os ativos */
void listarProdutos(void) {
    FILE *f = fopen(ARQ_BINARIO, "rb");
    Produto p;
    int qtd = 0;

    printf("\n=== LISTA DE PRODUTOS ===\n");
    if (f == NULL) {
        printf("\n[!] Nenhum produto cadastrado.\n");
        pausar();
        return;
    }
    cabecalhoTabela();
    while (fread(&p, sizeof(Produto), 1, f) == 1) {
        if (p.status == STATUS_ATIVO) {
            linhaTabela(&p);
            qtd++;
        }
    }
    fclose(f);
    if (qtd == 0) printf("(nenhum produto ativo)\n");
    else          printf("\nTotal: %d produto(s).\n", qtd);
    pausar();
}

/* Extra: ALTERAR dados de um produto */
void alterarProduto(void) {
    Produto p;
    int codigo, pos;
    char opcao[8];

    printf("\n=== ALTERAR PRODUTO ===\n");
    codigo = lerInteiro("Informe o codigo: ", 1);
    pos = buscarPosicao(codigo, &p);
    if (pos < 0) {
        printf("\n[!] Produto com codigo %d nao encontrado.\n", codigo);
        pausar();
        return;
    }

    exibirProduto(&p);
    printf("\n(Responda S para alterar o campo, qualquer outra tecla mantem)\n");

    lerTexto("Alterar nome? (S/N): ", opcao, sizeof(opcao));
    if (toupper((unsigned char)opcao[0]) == 'S')
        lerTexto("  Novo nome: ", p.nome, TAM_NOME);

    lerTexto("Alterar categoria? (S/N): ", opcao, sizeof(opcao));
    if (toupper((unsigned char)opcao[0]) == 'S')
        lerTexto("  Nova categoria: ", p.categoria, TAM_CATEGORIA);

    lerTexto("Alterar preco? (S/N): ", opcao, sizeof(opcao));
    if (toupper((unsigned char)opcao[0]) == 'S')
        p.preco = lerReal("  Novo preco (R$): ", 0.0f);

    if (regravarNaPosicao(pos, &p))
        printf("\n[OK] Produto atualizado.\n");
    else
        printf("\n[ERRO] Falha ao atualizar o arquivo.\n");
    pausar();
}

/* Extra: ENTRADA / SAÍDA de estoque */
void movimentarEstoque(void) {
    Produto p;
    int codigo, pos, tipo, qtd;

    printf("\n=== MOVIMENTAR ESTOQUE ===\n");
    codigo = lerInteiro("Informe o codigo: ", 1);
    pos = buscarPosicao(codigo, &p);
    if (pos < 0) {
        printf("\n[!] Produto com codigo %d nao encontrado.\n", codigo);
        pausar();
        return;
    }

    printf("\nProduto: %s | Estoque atual: %d un.\n", p.nome, p.quantidade);
    printf("1 - Entrada (adicionar)\n2 - Saida (remover)\n");
    do {
        tipo = lerInteiro("Tipo: ", 1);
    } while (tipo != 1 && tipo != 2);

    qtd = lerInteiro("Quantidade: ", 1);

    if (tipo == 2 && qtd > p.quantidade) {
        printf("\n[!] Estoque insuficiente (disponivel: %d).\n", p.quantidade);
        pausar();
        return;
    }
    p.quantidade += (tipo == 1) ? qtd : -qtd;

    if (regravarNaPosicao(pos, &p))
        printf("\n[OK] Novo estoque: %d un.\n", p.quantidade);
    else
        printf("\n[ERRO] Falha ao atualizar o arquivo.\n");
    pausar();
}

/* 4. EXCLUIR (exclusão lógica) */
void excluirProduto(void) {
    Produto p;
    int codigo, pos;
    char conf[8];

    printf("\n=== EXCLUIR PRODUTO ===\n");
    codigo = lerInteiro("Informe o codigo: ", 1);
    pos = buscarPosicao(codigo, &p);
    if (pos < 0) {
        printf("\n[!] Produto com codigo %d nao encontrado.\n", codigo);
        pausar();
        return;
    }

    exibirProduto(&p);
    lerTexto("\nConfirma a exclusao? (S/N): ", conf, sizeof(conf));
    if (toupper((unsigned char)conf[0]) != 'S') {
        printf("\nOperacao cancelada.\n");
        pausar();
        return;
    }

    p.status = STATUS_EXCLUIDO;          /* NÃO remove do arquivo: só marca */
    if (regravarNaPosicao(pos, &p))
        printf("\n[OK] Produto excluido (logicamente).\n");
    else
        printf("\n[ERRO] Falha ao atualizar o arquivo.\n");
    pausar();
}

/* 3. EXPORTAR PARA TXT */
void exportarTxt(void) {
    FILE *bin = fopen(ARQ_BINARIO, "rb");
    FILE *txt;
    Produto p;
    int qtd = 0;
    float totalGeral = 0.0f;

    printf("\n=== EXPORTAR PARA TXT ===\n");
    if (bin == NULL) {
        printf("\n[!] Nao ha registros para exportar.\n");
        pausar();
        return;
    }
    txt = fopen(ARQ_TEXTO, "w");
    if (txt == NULL) {
        fclose(bin);
        printf("\n[ERRO] Nao foi possivel criar %s.\n", ARQ_TEXTO);
        pausar();
        return;
    }

    fprintf(txt, "RELATORIO DE ESTOQUE\n");
    fprintf(txt, "===========================================================================\n");
    fprintf(txt, "%-6s %-30s %-18s %6s %12s\n",
            "COD", "NOME", "CATEGORIA", "QTD", "PRECO (R$)");
    fprintf(txt, "---------------------------------------------------------------------------\n");

    while (fread(&p, sizeof(Produto), 1, bin) == 1) {
        if (p.status == STATUS_ATIVO) {
            fprintf(txt, "%-6d %-30.30s %-18.18s %6d %12.2f\n",
                    p.codigo, p.nome, p.categoria, p.quantidade, p.preco);
            totalGeral += p.preco * p.quantidade;
            qtd++;
        }
    }
    fprintf(txt, "---------------------------------------------------------------------------\n");
    fprintf(txt, "Total de produtos ativos : %d\n", qtd);
    fprintf(txt, "Valor total em estoque   : R$ %.2f\n", totalGeral);

    fclose(bin);
    fclose(txt);
    printf("\n[OK] %d produto(s) exportado(s) para \"%s\".\n", qtd, ARQ_TEXTO);
    pausar();
}

/* =========================================================================
 *                                 MENU
 * ========================================================================= */

int menu(void) {
    printf("\n=====================================================\n");
    printf("        CONTROLE DE ESTOQUE DE PRODUTOS\n");
    printf("=====================================================\n");
    printf("  Produtos ativos cadastrados: %d\n", contarAtivos());
    printf("-----------------------------------------------------\n");
    printf("  1 - Cadastrar produto\n");
    printf("  2 - Consultar produto por codigo\n");
    printf("  3 - Listar produtos\n");
    printf("  4 - Alterar produto\n");
    printf("  5 - Movimentar estoque (entrada/saida)\n");
    printf("  6 - Excluir produto\n");
    printf("  7 - Exportar para arquivo TXT\n");
    printf("  0 - Sair\n");
    printf("-----------------------------------------------------\n");
    return lerInteiro("Escolha uma opcao: ", 0);
}

int main(void) {
    int opcao;
    do {
        opcao = menu();
        switch (opcao) {
            case 1: cadastrarProduto();   break;
            case 2: consultarProduto();   break;
            case 3: listarProdutos();     break;
            case 4: alterarProduto();     break;
            case 5: movimentarEstoque();  break;
            case 6: excluirProduto();     break;
            case 7: exportarTxt();        break;
            case 0: printf("\nEncerrando o sistema. Ate logo!\n"); break;
            default: printf("\n[!] Opcao invalida.\n");
        }
    } while (opcao != 0);
    return 0;
}
