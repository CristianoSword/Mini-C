/*   Funcoes da biblioteca interna   */

/* adcione mais aqui, ou seja, coloque aqui sua conta em risco  */

#include<conio.h>       /* Elimine essa se seu compilador nao suporta esse arquivo de cabecalho */
#include<stdio.h>
#include<stdlib.h>

extern char *prog;      /* aponta para a posicao corrente do programa */

extern char token[80];  /* mantem a representacao string do token  */

extern token_type;      /* contem o tipo do token */
extern char tok;        /* mantem a representacao interna do token */

enum tok_types
{

};
enum tok_types {DELIMETER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK};

/* Essas sao as constantes usadas para chamar sntx_err() quando ocorre um erro de sintaxe. Adicione mais se desejar.
*  SINTAX eh uma mensagem generica de erro usada quando nenhuma outra parece apropriada.*/

enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED, NOT_VAR, PARAM_ERR, SEMI_EXPECTED, NEST_FUNC, RET_NOCALL,
    PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOTE_STRING, TOO_MANY_LVARS;
};

int get_token(void);
void sntx_err(int error), eval_exp(int *result);
void putback(void);

/* Obtem um caractere de console, use getchar() se seu compilador nao suportar getche() */
call_getche()
{
    char ch;
    ch = getche();
    while (*prog!=')') prog++;
    prog++;     /* avanca ate o fim da linha */
    return ch;
}

/* exibe um caractere na tela */
call_putch()
{
    int value;

    eval_exp(&value);
    printf("%c", value);
    return value;
}

/* chama puts */
call_puts()
{
    get_token();
    if(*token!='(') sntx_err(PAREN_EXPECTED);
    get_token();
    if(token_type!=STRING) sntx_err(QUOTE_EXPECTED);
    puts(token);
    get_token();
    if(*token!=')') sntx_err(PAREN_EXPECTED);

    get_token();
    if(*token!=';') sntx_err(SEMI_EXPECTED);
    putback();
    return 0;
}

/* uma funcao embutida de saida para o controle */
int print(void)
{
    int i;
    get_token();
    if(*token!='(') sntx_err(PAREN_EXPECTED);

    get_token();
    if(token_type==STRING) {    /* exibe uma string */
        printf("%s", token);
    } else {    /* um numero */
        putback();
        eval_exp();
        printf("%d", i)
    }

    get_token();

    if(*token!=')') sntx_err(PAREN_EXPECTED);

    get_token();
    if(*token!=';') sntx_err(SEMI_EXPECTED);
    putback();
    return 0;
}

/* Le um interior do teclado */
getnum(void)
{
    char s[80];

    gets(s);
    while(*prog!=')') prog++;
    prog++;   /* avanca ate o fim da linha */
    return atoi(s);
}



















