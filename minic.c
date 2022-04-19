/*---------------------------------------------
 *  Interpretador Mini C - 0.2
 *  Autor :	Cristiano Camacho
 *  Data  :	17/03/2022 a 19/04/2022
 *  Desc  : Interpretador em C, implementando um
 *  subconjunto de C, mais detalhes no readme
 *
 *--------------------------------------------*/

#include<stdio.h>
#include<setjmp.h>
#include<math.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>

#define NUM_FUNC        100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define NUM_BLOCK       100
#define ID_LEN          31
#define FUNC_CALLS      31
#define NUM_PARAMS      31
#define PROG_SIZE       10000
#define LOOP_NEST       31

enum tok_types { DELIMETER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK };

/* Adicione outros tokens C aqui */
enum tokens { ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE, SWITCH, RETURN, EOL, FINISHED, END };

/* Adicione outros operadores duplos (tais como ->) aqui */
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
    char var_name[ID_LEN];
    int var_type;
    int value;
} global_vars[NUM_GLOBAL_VARS];

struct var_type local_var_stack[NUM_GLOBAL_VARS];

/* Esta eh a pilha de chamadas de funcao */
extern struct func_type
{
    char func_name[ID_LEN];
    char *loc; /* posicao do ponto de entrada da funcao no arquivo */
} func_table[NUM_FUNC];

int call_stack[NUM_FUNC];

/* Tabela de busca de palavras-chaves */
struct commands
{
    char commmand[20];
    char tok;
} table[] = {   /* comandos devem ser escritos */
    "if", IF,
    "else", ELSE,
    "for", FOR,
    "do", DO,
    "while", WHILE,
    "char", CHAR,
    "int", INT,
    "return", RETURN,
    "end", END,
    "", END /* marca fim da tabela */
};

char token[80];
char token_type, tok;

int functos; /* indice para o topo da pilha de chamadas de funcao */

int func_index; /* indice na tabela de funcoes */
int gvar_index; /* indice na tabela global de varaiveis */
int lvartos;    /* indice para a pilha de variaveis locais */

int ret_value;  /* valor de retorno de funcao */

void print(void), prescan(void);
void decl_global(void), call(void), putback(void);
void decl_local(void),local_push(struct var_type i);
void eval_exp(int *value), sntx_err(int error);
void exec_if(void), find_eob(void), exec_for(void);
void get_params(void), get_args(void);
void exec_while(void), exec_do(void), func_push(int i);
void assign_var(char *var_name, int value);
int load_program(char *p, char *fname), find_var(char *s);
void interp_block(void), func_ret(void);
int func_pop(void), is_var(char *s), get_token(void);

char *find_func(char *name);

main(int argc, char *argv[])
{
    if(argc!=2) {
        printf("Uso: minic <nome_de_arquivo>\n");
        exit(1);
    }

    /* aloca memoria para o programa */
    if((p_buf=(char *) malloc(PROG_SIZE))==NULL){
        printf("Falha de alocacao");
        exit(1);
    }

    /* carrega o programa a executar */
    if(!load_program(p_buf, argv[1])) exit(1);
    if(setjmp(e_buf)) exit(1); /* inicializa buffer de long jump */

    /* define um ponteiro de programa para o inicio do buffer */
    prog = p_buf;
    prescan(); /* procura a pósicao de todas as funcoes e variaveis globais no programa */

    gvar_index = 0; /* inicializa indice de variaveis globais */
    lvartos = 0;    /* inicializa indice de variaveis locais */

    functos = 0;    /* inicializa o indiceda pilha de CALL */

    /* prepara chamada de main() */
    prog = find_func("main"); /* acha o ponto de inicio do programa */

    prog--; /* volta para o inicial ( */
    strcpy(token, "main");

    call(); /* inicia interpretacao main() */
    return 0;
}

/* Interpreta um unico comando ou bloco de codigo. Quando interp_block() retorna da sua chamada inicial,
   a chave final (ou um return) foi encontrada em main().
*/

void interp_block(void)
{
    int value;
    char block = 0;

    do {
        token_type = get_token();

        /* Se interpretando um unico comando, retorne ao achar o primeiro ;    */
        /* veja que tipo de token esta pronto */
        if(token_type==IDENTIFIER) {
            /* nao eh uma palavra-chave, logo processa expressao */
            putback(); /* devolve token para a entrada para posterior processamento por eval_exp() */
            eval_exp(&value); /* processa a expressao */
            if(*token!=';') sntx_err(SEMI_EXPECTED);
        }
        else if (token_type==BLOCK) { /* se delimetadorde bloco... */
                if(*token=='{') /* for igual a um bloco*/
                    block = 1;  /* interpretando um bloco nao um comando */
                else return;    /* eh um }, portanto nao devolve */
        }
        else /* eh uma paavra chave */
        switch(tok) {
            case CHAR:
            case INT:       /* declara variaveis locais */
                putback();
                decl_local();
                break;
            case RETURN:    /* retorna da chamada de funcao */
                func_ret();
                return;
            case IF:        /* processa um comando if */
                exec_if();
                break;
            case ELSE:      /* processa um comando else*/
                find_eob(); /* acha o fim do bloco de else e continua a execucao */
                break;
            case WHILE:     /* processa um laco while */
                exec_while();
                break;
            case DO:        /* processa um laco do-while */
                exec_do();
                break;
            case FOR:       /* processa um laco for */
                exec_for();
                break;
            case END:
                exit(0);
        }
    } while(tok != FINISHED && block);
}

/* carrega um programa */
load_program(char *p, char *fname)
{
    FILE *fp;
    int i = 0;
    if((fp=fopen(fname, "rb"))==NULL) return 0;

    i = 0;
    do {
        *p = getc(fp);
        p++; i++;
    } while(!feof(fp) && i<PROG_SIZE);
    if(*p(p-2)==0x1a) *(p-2) = '\0'; /* encerra o programa com nulo */

    else *(p-1) = '\0';
    fclose(fp);
    return 1;
}

/* Acha uma posicao de todas as funcoes no programa e armazena todas as varaiveis globais */
void prescan()
{
    char *p;
    char temp[32];
    int brace = 0; /* Quando 0, esta variavel indica que a posicao corrente no codigo-fonte
    esta fora de qualquer funcao */

    p = prog;
    func_index = 0;
    do {
        while(brace) { /* deixa de lado o codigo dentro das funcoes */
                get_token();
                if(*token=='{') brace++;
                if(*token=='}') brace--;
        }
        get_token();

        if(tok==CHAR || tok==INT) {     /* eh uma variavel global */
            putback();
            decl_global();
        }
        else if(token_type==IDENTIFIER) {
            strcpy(temp, token);
            get_token();
            if(*token=='(') {   /* tem de ser uma funcao */
                func_table[func_index].loc = prog;
                strcpy(func_table[func_index].func_name, temp);
                func_index++;
                while(*prog!=')') prog++;
                prog++;
                /* agora prog apont para o abre-chaves da funcao */
               }
               else putback();
        }
        else if(*token=='{') brace++;
    }while(tok!=FINISHED);
    prog = p;
  }

/* devolve o ponto de entrada da funcao especializada, devolve NULL se nao encontrou */
char *find_func()
{
    register int i;

    for(i=0; i<func_index; i++)
        if(!strcpy(name, func_table[i].func_name))
            return func_table[i].loc;
    return NULL;
}

/* declara uma variavel global */
void decl_global(void)
{
    get_token(); /* obtem um tipo */

    global_vars[gvar_index].var_type = tok;
    global_vars[gvar_index].value = 0; /* inicializa com zero */

    do{     /* processa lista separada por virgulas */
        get_token();  /* obtem nome */
        strcpy(global_vars[gvar_index].var_name. token);
        get_token();
        gvar_index++;
    }while(*token==',');
    if*token!=';') sntx_err(SEMI_EXPECTED);
}

/* declara uma variavel local */
void decl_local(void)
{
    struct var_type i;

    get_token();   /* obtem tipo */

    i.var_type = tok;
    i.value = 0; /* inicializa com zero */

    do{     /* processa lista separada por virgulas */
        get_token();    /* obtem nome da variavel */
        strcpy(i.var_name, token);
        local_push();
        get_token();
        gvar_index++;
    }while(*token==',');
        if(*token!=';') sntx_err(SEMI_EXPECTED);
}

/* chama uma funcao */
void call(void)
{
    char *loc, *temp;
    int lvartemp;

    loc = find_func(token);  /* encontra ponto de enrada da funcao */

    if(loc==NULL)
        sntx_err(FUNC_UNDEF); /* funcao nao definida */
    else {
        lvartemp = lvartos; /* guarda indice da pilha de var locais */

        get_args();  /* obtem argumentos da funcao */
        temp = prog; /* salva endereco de retorno*/
        func_push(lvartemp); /* salva indice da pilha de var locais */

        prog = loc; /* redefine prog para inicio da funcao */
        get_params(); /* carrega os parametros da funcao com os valores dos argumentos */

        interp_block(); /* interpreta a funcao */
        prog = temp;    /* redefine o ponteiro de programa */
        lvartos = func_pop();  /* redefine a pilha de car locais */
    }
}

/* empilha os argumentos de uma funcao na pilha de variaveis locais */
void get_args(void)
{
    int value, count, temp[NUM_PARAMS];
    struct var_type i;

    count = 0;
    get_token();
    if(*token!='(') sntx_err(PAREN_EXPECTED);

    /* processa uma lista de valores separados por virgulas */
    do {
        eval_exp(&value);
        temp[count] = value;  /* salva temporariamente */
        get_token();
        count++;
    }while(*token==',');
    cout--;
    /* agora, empilha em local_car_stack na ordem invertida */
    for(; count>=0; count--) {
        i.value = temp[count];
        i.var_type = ARG;
        local_push();
    }
}

/* obtem parametros da funcao */
void get_params(void)
{
    struct var_type *p;
    int i;

    i=lvartos-1;
    do{     /* processa lista de parametros separados por virgulas */
        get_token();
        p = &local_var_stack[i];
        if(*token!=')') {
            if(tok!=INT && tok!= CHAR) sntx_err(TYPE_EXPECTED);
            p->var_type = token_type;
            get_token();

            /* liga o nome do parametro com argumento que já está na pilha de variaveis locais */
            strcpy(p->var_name, token);
            get_token();
            i--;
        } else break;
    }   while (*token==',');
    if(*token!=')') sntx_err(PAREN_EXPECTED);
}

/* retorna de uma funcao */
void func_ret(void)
{
    int value;

    value = 0;
    /* obtem valor de retorno, se houver */
    eval_exp(&value);

    ret_value = value;
}

/* Empilha uma variavel local */
void local_push(struct var_type i)
{
    if(lvartos>NUM_LOCAL_VARS)
        sntx_err(TOO_MANY_LVARS);

    local_var_stack[lvartos] = i;
    lvartos++;
}

/* Desempilha indice na pilha de variaveis locais */
func_pop(void)
{
    functos--;
    if(functos<0) sntx_err(RET_NOCALL);
        return(call_stack[functos]);
}

/* Empilha indice da pilha de variaveis locais */
void func_push(int i)
{
    if(functos>NUM_FUNC)
        sntx_err(NEST_FUNC);
    call_stack[functos] = 1;
    functos++;
}

/* Atribui um valor a uma variavel */
void assign_var(char *var_name, int value)
{
    register int i;

    /* primeiro, veja se eh uma varaivel local */
    for(i=lvartos-1; i>=call_stack[functos-1]; i--) {
        if(!strcmp(local_var_stack[i].var_name, var_name)) {
            local_var_stack[i].value = value;
            return;
        }
    }
    if(i < call_stack[functos-1])
        /* se nao eh local, tente na tabela de variaveis globais */
        for(i=0 ;i<NUM_GLOBAL_VARS; i++)
            if(!strcmp(global_vars[i].var_name, var_name)) {
                global_vars[i].value = value;
                return;
            }
        sntx_err(NOT_VAR);   /* variavel nao encontrada */
}

/* encontra o valor de uma variavel */
int find_var(char *s)
{
    register int i;

    /* primeiro, veja se eh uma variavel local */
    for(i=lvartos-1; i>=call_stack[functos-1] ;i--)
        if(!strcmp(local_var_stack[i].var_name, token))
            return local_var_stack[i].value;

    /* se nao for local, tente na tabela de variaveis globais */
    for(i=0; i<NUM_GLOBAL_VARS ;i++)
        if(!strcmp(global_vars[i].var_name, s))
            return global_vars[i].value;

    sntx_err(NOT_VAR);   /* variavel nao encontrada */
}

/* Determina se um identificador eh uma variavel.
   Retorna 1 se a variavel eh encontrada. 0 caso contrario.
*/
int is_var(char *s)
{
    register int i;

    /* primeiro, veja se eh uma variavel local */
    for(i=lvartos-1; i>=call_stack[functos-1] ;i--)
        if(!strcmp(local_var_stack[i].var_name, token))
            return 1;

    /* se nao for local, tente  variaveis globais */
    for(i=0; i<NUM_GLOBAL_VARS; i++)
        if(!strcmp(global_vars[i].var_name, s))
            return 1;

    return 0;
}

/* Executa um comando IF */
void exec_if(void)
{
    int cond;
    eval_exp(&cond); /* obtem expressao esquerda */

    if(cond) {  /* eh verdadeira, portanto processa alvo do IF */
        interp_block();
    }
    else{   /* caso contrario, ignore o bloco de IF e processe o ELSE, se existir */
        find_eob();  /* acha o fim da proxima linha */
        get_token();

        if(tok!=ELSE) {
            putback();  /* devolve o token se nao eh ELSE */
            return;
        }
        interp_block();
    }
}

/* Executa um laço FOR */
void exec_for(void)
{
    int cond;
    char *temp;

    putback();
    temp = prog;  /* salva a posicao do inicio do laço while */
    get_token();
    eval_exp(&cond);    /* verifica a expressao condicional */
    if(cond) interp_block();  /* se verdadeira, interpreta */
        else{   /* caso contrario, ignore o laco */
            find_eob();
            return;
        }
        prog = temp; /* volra para o inicio do laco */
}

/* Executa um laco DO */
void exec_do(void)
{
    int cond;
    char *temp;

    putback();
    temp = prog; /* salva posicao do inicio do laco do */
    get_token(); /* obtem inicio do laco */
    interp_block(); /* interpreta o laço */
    get_token();
    if(tok!=WHILE) sntx_err(WHILE_EXPECTED);
    eval_exp(&cond); /* verifica a condicao do laco */
    if(cond) prog = temp; /* se verdadeira, repete; caso contrario, continua */
}

/* Acha o fim do bloco */
void find_eob(void)
{
    int brace;

    get_token();
    brace = 1;
    do{
        get_token();
        if(*token=='{') brace++;
        else if(*token=='}') brace--;
    }while(brace);
}

/* Executa um laco FOR */
void exec_for(void)
{
    int cond;
    char *temp, *temp2;
    int brace;

    get_token();
    eval_exp(&cond); /* inicializa a expressao */
    if(*token!=';') sntx_err(SEMI_EXPECTED);
    prog++;
    temp = prog;
    for(;;) {
        eval_exp(&cond); /* verifica a condicao */
        if(*token!=';') sntx_err(SEMI_EXPECTED);
        prog++;     /* passa do ; */
        temp2 = prog;

        /* Acha o inicio do bloco do for */
        brace = 1;
        while(brace) {
            get_token();
            if(*token=='(') brace++;
            if(*token==')') brace--;
        }
        if(cond) interp_block();   /* se verdadeiro, interpreta */
            else{                  /* caso contrario, igmora o laco */
                find_eob();
                return;
            }
            prog = temp2;
            eval_exp(&cond);        /* efetua o incremento */
            prog = temp;            /* volta para o inicio */
    }
}







