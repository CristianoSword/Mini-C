/* Analisador recursivo descendente de expressoes inteiras que pode
*  incluir variaveis e chamadas de funcoes*/

#include<setjmp.h>
#include<math.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

#define NUM_FUNC        100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define ID_LEN          31
#define FUNC_CALLS      31
#define PROG_SIZE       10000
#define FOR_NEST        31

enum tok_types { DELIMETER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK };

enum tokens { ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE, SWITCH, RETURN, EOL, FINISHED, END };

enum double_ops { LT=1, LE, GT, GE, EQ, NE };

/* Essas sao as constantes usadas para chamar sntx_err() quando ocorre um erro de sintaxe. Adicione mais se desejar.
*  SINTAX eh uma mensagem generica de erro usada quando nenhuma outra parece apropriada.*/

enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED, NOT_VAR, PARAM_ERR, SEMI_EXPECTED, UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
    NEST_FUNC, RET_NOCALL, PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOTE_TEMP, TOO_MANY_LVARS
};

extern char *prog ; /* posicao corrente no codigo fonte */
extern char *p_buf; /* aponta para o inicio da area de carga do programa */

extern jmp_buf e_buf; /* mantem ambiente para longjmp() */

/* Uma matriz destas estruturas manterá a informação associada com as variaveis globais */

extern struct var_type
{
    char var_name[32];
    enum variable_type var_type;
    int value;
} global_vars[NUM_GLOBAL_VARS];

/* Esta eh a pilha de chamadas de funcao */
extern struct func_type
{
    char func_name[32];
    char *loc; /* posicao do ponto de entrada da funcao no arquivo */
} func_stack[NUM_FUNC];

/* Tabela de palavras reservadas */
extern struct commands
{
    char command[20];
    char tok;
} table[];

/* Funcoes da "Biblioteca padrao" sao declaradas aqui para que possam
*  ser colocadas na tabela interna de funcoes que segue. */

int call_getche(void), call_putch(void);
int call_puts(void), print(void), getnum(void);

struct intern_func_type
{
    char *f_name; /* nome da funcao */
    int (*p)();  /* ponteiro para funcao */
}intern_func[] ={
    "getche", call_getche,
    "putch", call_putch,
    "puts", call_puts,
    "print", print,
    "getnum", getnum,
    "", 0 /* NULL termina a lista */
};

extern char token[80];   /* representacao string do token */
extern char token_type;  /* contem o tipo do token */
extern char tok;         /* representacao interna do token */

extern int ret_value;   /* valor de retorno de funcao */

void eval_exp(int *value);
void eval_exp0(int *value);
void eval_exp1(int *value);
void eval_exp2(int *value);
void eval_exp3(int *value);
void eval_exp4(int *value);
void eval_exp5(int *value);
void atom(int *value);
void sntx_err(int error), putback(void);
void assign_var(char *var_name, int value);
int isdelim(char c), look_up(char *s), iswhite(char c);
int find_var(char c);
int internal_func(char *s);
int is_var(char *s);
char *find_func(char *name);
void call(void);

/* ponto de entrada do analisador */
void eval_exp(int *value)
{
    get_token();
    if(!*token){
        sntx_err(NO_EXP);
        return;
    }
    if(*token==';'){
        *value = 0; /* expressao vazia */
        return;
    }
    eval_exp0(value);
    putback(); /* devolve ultimo token lido para a entrada */
}

/* Processa uma expressao de atribuicao */
void eval_exp0(int *value)
{
    char temp[ID_LEN]; /* guarda nome da variavel que esta recebendo a atribuicao */
    register int temp_tok;

    if(token_type==IDENTIFIER) {
        if(is_var(token)){ /* se eh um variavel veja se eh uma atribuicao */
            strcpy(temp, token);
            temp_tok = token_type;
            if(*token=='=') { /* eh um atribuicao */
                get_token();
                eval_exp0(value); /* obtem valor a atribuir*/
                assign_var(temp, *value); /*atribui valor*/
                return;
            }
            else{ /* nao eh uma atribuicao */
                putback();
                strcpy(token, temp);
                token_type = temp_tok;
            }

        }
    }
    eval_exp1(value);
}

/* Essa matriz eh usada por eval_exp1(). Como alguns compiladores nao permitem inicializar uma
*  matriz dentro de uma funcao, ela eh definida como uma variavel global */

char relops[7] = { LT, LE, GT, GE, EQ, NE, 0 };

/* Processa operadores relacionais */
void eval_exp1(int *value)
{
    int partial_value;
    register char op;

    eval_exp2(value);
    op = *token;

    if(strchr(relops, op)) {
        get_token();
        eval_exp2(&partial_value);
            switch(op) { /* efetua a operacao relacional */
            case LT:
                *value = *value < partial_value;
                break;
            case LE:
                *value = *value <= partial_value;
                break;
            case GT:
                *value = *value > partial_value;
                break;
            case GE:
                *value = *value >= partial_value;
                break;
            case EQ:
                *value = *value == partial_value;
                break;
            case NE:
                *value = *value != partial_value;
                break;
            }
        }
}

/* Soma ou subtrai dois termos */
void eval_exp2(int *value)
{
    register char op;
    int partial_value;

    eval_exp3(value);
    while((op = *token) == '+' || op == '-') {
        get_token();
        eval_exp3(&partial_value);
        switch (op) { /* soma ou subtrai */
            case '-':
                *value = *value - partial_value;
                break;
            case '+':
                *value = *value + partial_value;
                break;
        }
    }
}

/* Multiplica ou divide dois fatores */
void eval_exp3(int *value)
{
    register char op;
    int partial_value, t;

    eval_exp4(value);
    while((op = *token) == '*' || op == '/' || op == '%') {
        get_token();
        eval_exp4(&partial_value);
        switch (op) { /* mul, div ou modulo */
            case '*':
                *value = *value * partial_value;
                break;
            case '/':
                *value = *value / partial_value;
                break;
            case '%':
                t = (*value) / partial_value;
                *value *value - (t * partial_value);
                break;
        }
    }
}

/* eh um + ou - unario */
void eval_exp4(int *value)
{
    register char op;

    op = '\0';
    if(*token=='+' || *token=='-'){
        op = *token;
        get_token();
    }
    eval_exp5(value);
    if(op=='-') *value = - (*value);
}

/* Processa espressoes com parenteses */
void eval_exp5(int *value)
{
    if((*token == '(')) {
        get_token();
        eval_exp0(value); /* obtem subexpressao */
        if(*token != ')') sntx_err(PAREN_EXPECTED);
        get_token();
       }
       else
        atom(value);
}

/* Acha valor de numero, variavel ou funcao */
void atom(int *value)
{
    int i;

    switch(token_type) {
    case IDENTIFIER:
        i = internal_func(token);
        if(i!= -1) { /* chama funcao da "biblioteca padrao" */
                *value = (*intern_func[i.p])();
        }
        else
        if((find_func(token))) { /* chama a funcao definida pelo usuario */
                call();
                *value = ret_value;
        }
        else *value = find_var(token); /* obtem valor da variavel */
        get_token();
        return;
    case NUMBER: /* eh uma constante numerica */
        *value = atoi(token);
        get_token();
        return;
    case DELIMETER: /* veja se eh uma constante caractere */
        if(*token=='\''){
            *value=*prog;
            prog++;
            if(*prog!='\'') sntx_err(QUOTE_EXPECTED);
            prog++;
            get_token();
        }
        return;
    default:
        if(*token==')') return; /* processa expressao vazia */
        else sntx_err(SYNTAX); /* erro de sintaxe */
    }
}

/* Exibe uma mensagem de erro */
void sntx_err(int error)
{
    char *p, *temp;
    register int i;
    int linecount = 0;

    static char*e[] = {
        "erro de sintaxe",
        "parenteses desbalanceados",
        "falta uma expressao",
        "esperado sinal de igual",
        "nao eh uma variavel",
        "erro de parametro",
        "esperado ponto-e-virgula",
        "chaves desbalanceadas",
        "funcao nao definida",
        "esperado identificador de tipo",
        "excessivas chamadas aninhadas de funcao",
        "return sem chamada",
        "esperando parenteses",
        "esperando whiles",
        "esperando fechar aspas",
        "nao eh uma string",
        "excessivas variaveis locais",
    };
    printf("%s", e[error]);
    p = p_buf;
    while(p != prog) { /*encotra linha do erro */
        p++;
        if(*p == '\r'){
            linecount++;
        }
    }
    printf(" na linha %d\n", linecount);

    temp = p; /* exibe linha contendo erro */
    for(i=0; i<20 && p>p_buf && *p!='\n'; i++, p--);
    for(i=0; i<30 && p<=temp; i++, p++); printf("%c", *p);

    longjmp(e_buf, 1); /* retorna para um ponteiro seguro */
}

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
    if(token_type == TEMP) {
        tok = look_up(token); /* converte para representacao interna */
        if(tok) token_type = KEYWORD;
        else token_type = IDENTIFIER;
    }
    return token_type;
 }

/* Devolve um token para a entrada */
void putback(void)
{
    char *t;

    t = token;
    for(; *t; t++) prog--;
}

/* Procura pela representacao interna de um token na tabela de tokens. */

look_up(char *s)
{
    register int i;
    char *p;

    /* converte para minuscula */
    p = s;
    while(*p){ *p = tolower(*p); p++; }

    /* verifica se o token está na tabela */
    for(i=0; *table[i].command; i++)
        if(!strcmp(table[i].command,s)) return table[i].tok;
    return 0; /* comando desconhecido */
}

/* Retorna indice de funcao da biblioteca interna ou -1 se nao encotrada */
internal_func(char *s)
{
    int i;

    for(i=0; internal_func[i].f_name[0]; i++){
        if(!strcmp(internal_func[i].f_name, s)) return i;
    }
    return -1;
}

/* Retorna verdadeiro se c é um delimetador */
isdelim(char c)
{
    if(strchr(" !;,+-<>'/*%^=()", c)) || c == 9 || c == '\r' || c == 0) return 1;
    return 0;
}

/* Retorna 1 se c eh o espaço ou tabulação */
iswhite(char c)
{
    if(c==' ' || c== '\t') return 1;
    else return 0;
}










