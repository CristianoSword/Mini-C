/*---------------------------------------------
 *  Interpretador Little C - 0.1
 *  Autor :	Cristiano Camacho
 *  Data  :	17/03/2022 a 12/04/2022
 *  Desc  : Interpretador em C, implementando um
 *  subconjunto de C, mais detalhes no readme
 *
 *--------------------------------------------*/

#include<stdio.h>

void main(void)
{
    int a,b=6;
    printf("isso eh uma expressao %d", a=4+5);

    switch(a>1)
        case 1: if(b>5){
            printf("\n switch");
        }

    return 0;
}

/* Dados globais e enumeracao da funcao get_token */

char *prog; /* aponta para a posicao corrente no codigo fonte */
extern char *p_buf; /* aponta para o inicio do buffer de programa */

char token[80]; /* contem a representacao do token como string */
char token_type; /* contem o tipo do token */
char tok;   /* contema representacao interna do token se eh uma palavra-chave */

enum tok_types {DELIMETER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK};
enum double_ops {LT=1, LE, GT, GE, EQ, NE};

/* Essas sao as constantes usadas para chamar sntx_err() quando ocorre um erro de sintaxe. Adicione mais se desejar.
*  SINTAX eh uma mensagem generica de erro usada quando nenhuma outra parece apropriada.*/

enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED, NOT_VAR, PARAM_ERR, SEMI_EXPECTED, NEST_FUNC, RET_NOCALL,
    PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOTE_TEMP, TOO_MANY_LVARS;
};



/* Obtem um token */
get_token(void)
 {
     register char *temp;

     token_type = 0; tok = 0;
     temp = token;
     *temp = '\0';

     /* ignora espaco vazio */
     while(iswhite(*prog) && *prog) ++prog;

     if(*prog=='\r') {
        ++prog;
        ++prog;
            /* ignora espaco vazio */
            while(iswhite(*prog) && *prog) ++prog;
     }

    if(*prog=='\0') { /* delimitadores de bloco */
        *token = '\0';
        tok = FINISHED;
    return (token_type = DELIMETER);
    }

    if(strchr("{}", *prog)){  /* delimetadores de bloco */
        *temp = *prog;
        temp++;
        *temp = '\0';
        prog++;
    return (TOKEN_type = BLOCK);
    }

    /* procura por comentarios */
    if(*prog == '/')
        if(*(prog+1) == '*')  /* eh um comentario */
        {
            prog += 2;
            do { /* procura por comentarios */
                while(*prog!='*') prog++;
                    prog++;
                }
                while(*prog!='/');
                prog++
        }

    if(strchr("!<>=", *prog)) /* eh ou pode ser um operador relacional */
    {
        switch (*prog)
        {
          case '=':if(*(prog+1)=='=')
            {
                prog++;prog++;
                *temp = EQ;
                temp++; *temp = EQ; temp++;
                *temnp = '\0';
            }break;
          case '!':if(*(prog+1)=='=')
            {
                prog++;prog++;
                *temp =  NE;
                temp++; *temp = NE; temp++;
                *temp = '\0';
            }break;
          case '<':if(*(prog+1)=='=')
            {
                prog++;prog++;
                *temp = LE; temp++; *temp = LE;
            }else{
                prog++;
                *temp = LT;
            }
            temp++;
            *temp = '\0';
            break;
              case '>':if(*(prog+1)=='=')
            {
                prog++;prog++;
                *temp = GE; temp++; *temp = GE;
            }else{
                prog++;
                *temp = GT;
            }
            temp++;
            *temp = '\0';
            break;
        }
        if(*token) return (token_type = DELIMITER);
    }

    if(strchr("+-*^/%=;(),'", *prog)){  /* delimetador */
        *temp = *prog;
        prog++; /* avanca para a proxima posicao */
        temp++;
        *temp = '\0';
    return (token_type = DELIMITER);
    }

    if(*prog=='"') { /* string entre aspas */
        prog++;
        while(*prog!='"'&& *prog!='\r') *temp++ = *prog++;
        if(*prog=='\r') sntx_err(SYNTAX);
        prog++; *temp = '\0';
    return (token_type = DELIMITER);
    }

    if(isdigit(*prog)) { /* numero */
        while(!isdelim(*prog)) *temp++ = *prog++;
        *temp = '\0';
    return(token_type = NUMBER);
    }

    if(isalpha(*prog)) {  /* variavel ou comando */
        while(!isdelim(*prog)) *temp++ = *prog++;
    token_type = TEMP;
    }

    *temp = '\0';

    /* verifica se uma string eh um comando ou uma variavel */
    if(token_type==TEMP) {
        tok = look_up(token); /* converte para representacao interna */
        if(tok) token_type = KEYWORD;
        else token_type = IDENTIFIER;
    }
    return token_type;
 }





























