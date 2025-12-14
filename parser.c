/* Recursive descent parser for integer expressions that can
*  include variables and function calls*/

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

enum tok_types { DELIMITER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK };

enum tokens { ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE, SWITCH, RETURN, EOL, FINISHED, END };

enum double_ops { LT=1, LE, GT, GE, EQ, NE };

/* These are the constants used to call sntx_err() when a syntax error occurs. Add more if desired.
*  SYNTAX is a generic error message used when no other seems appropriate.*/

enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED, NOT_VAR, PARAM_ERR, SEMI_EXPECTED, UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
    NEST_FUNC, RET_NOCALL, PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP, TOO_MANY_LVARS
};

extern char *prog;  /* current position in source code */
extern char *p_buf; /* points to the beginning of program load area */

extern jmp_buf e_buf; /* holds environment for longjmp() */

/* An array of these structures will hold the information associated with global variables */

extern struct var_type {
    char var_name[32];
    int var_type;
    int value;
} global_vars[NUM_GLOBAL_VARS];

/* This is the function call stack */
extern struct func_type
{
    char func_name[32];
    char *loc; /* position of function entry point in file */
} func_stack[NUM_FUNC];

/* Reserved word table */
extern struct commands
{
    char command[20];
    char tok;
} table[];

/* "Standard library" functions are declared here so they can
*  be placed in the internal function table that follows. */

int call_getche(void), call_putch(void);
int call_puts(void), print(void), getnum(void);

struct intern_func_type{
    char *f_name; /* function name */
    int (*p)();   /* pointer to function */
}intern_func[] ={
    "getche", call_getche,
    "putch", call_putch,
    "puts", call_puts,
    "print", print,
    "getnum", getnum,
    "", 0 /* NULL terminates the list */
};

extern char token[80];   /* string representation of token */
extern char token_type;  /* contains token type */
extern char tok;         /* internal representation of token */

extern int ret_value;    /* function return value */

void eval_exp(int *value), eval_exp1(int *value);
void eval_exp2(int *value);
void eval_exp3(int *value), eval_exp4(int *value);
void eval_exp5(int *value), atom(int *value);
void eval_exp0(int *value);
void sntx_err(int error), putback(void);
void assign_var(char *var_name, int value);
int isdelim(char c), look_up(char *s), iswhite(char c);
int find_var(char *s), get_token(void);
int internal_func(char *s);
int is_var(char *s);
char *find_func(char *name);
void call(void);

/* parser entry point */
void eval_exp(int *value)
{
    get_token();
    if(!*token){
        sntx_err(NO_EXP);
        return;
    }
    if(*token==';'){
        *value = 0; /* empty expression */
        return;
    }
    eval_exp0(value);
    putback(); /* return last read token to input */
}

/* Process an assignment expression */
void eval_exp0(int *value)
{
    char temp[ID_LEN]; /* stores name of variable receiving assignment */
    register int temp_tok;

    if(token_type==IDENTIFIER) {
        if(is_var(token)){ /* if it's a variable see if it's an assignment */
            strcpy(temp, token);
            temp_tok = token_type;
            get_token();
            if(*token=='=') { /* it's an assignment */
                get_token();
                eval_exp0(value); /* get value to assign*/
                assign_var(temp, *value); /*assign value*/
                return;
            }
            else{ /* not an assignment */
                putback();
                strcpy(token, temp);
                token_type = temp_tok;
            }

        }
    }
    eval_exp1(value);
}

/* This array is used by eval_exp1(). Since some compilers don't allow initializing an
*  array inside a function, it's defined as a global variable */

char relops[7] = { LT, LE, GT, GE, EQ, NE, 0 };

/* Process relational operators */
void eval_exp1(int *value)
{
    int partial_value;
    register char op;

    eval_exp2(value);
    op = *token;

    if(strchr(relops, op)) {
        get_token();
        eval_exp2(&partial_value);
            switch(op) { /* perform relational operation */
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

/* Add or subtract two terms */
void eval_exp2(int *value)
{
    register char op;
    int partial_value;

    eval_exp3(value);
    while((op = *token) == '+' || op == '-') {
        get_token();
        eval_exp3(&partial_value);
        switch (op) { /* add or subtract */
            case '-':
                *value = *value - partial_value;
                break;
            case '+':
                *value = *value + partial_value;
                break;
        }
    }
}

/* Multiply or divide two factors */
void eval_exp3(int *value)
{
    register char op;
    int partial_value, t;

    eval_exp4(value);
    while((op = *token) == '*' || op == '/' || op == '%') {
        get_token();
        eval_exp4(&partial_value);
        switch (op) { /* mul, div or modulo */
            case '*':
                *value = *value * partial_value;
                break;
            case '/':
                *value = (*value) / partial_value;
                break;
            case '%':
                t = (*value) / partial_value;
                *value = *value - (t * partial_value);
                break;
        }
    }
}

/* it's a unary + or - */
void eval_exp4(int *value)
{
    register char op;

    op = '\0';
    if(*token=='+' || *token=='-'){
        op = *token;
        get_token();
    }
    eval_exp5(value);
    if(op)
    if(op=='-') *value = - (*value);
}

/* Process expressions with parentheses */
void eval_exp5(int *value)
{
    if((*token == '(')) {
        get_token();
        eval_exp0(value); /* get subexpression */
        if(*token != ')') sntx_err(PAREN_EXPECTED);
        get_token();
       }
       else
        atom(value);
}

/* Find value of number, variable or function */
void atom(int *value)
{
    int i;

    switch(token_type) {
    case IDENTIFIER:
        i = internal_func(token);
        if(i!= -1) { /* call "standard library" function */
                *value = (*intern_func[i].p)();
        }
        else
        if(find_func(token)) { /* call user-defined function */
                call();
                *value = ret_value;
        }
        else *value = find_var(token); /* get variable value */
        get_token();
        return;
    case NUMBER: /* it's a numeric constant */
        *value = atoi(token);
        get_token();
        return;
    case DELIMITER: /* see if it's a character constant */
        if(*token=='\''){
            *value=*prog;
            prog++;
            if(*prog!='\'') sntx_err(QUOTE_EXPECTED);
            prog++;
            get_token();
            return;
        }
        if(*token==')') return; /* process empty expression */
        else sntx_err(SYNTAX); /* syntax error */
    default:
        sntx_err(SYNTAX); /* syntax error */
    }}

/* Display an error message */
void sntx_err(int error)
{
    char *p, *temp;
    register int i;
    int linecount = 0;

    static char *e[] = {
        "syntax error",
        "unbalanced parentheses",
        "missing expression",
        "expected equals sign",
        "not a variable",
        "parameter error",
        "expected semicolon",
        "unbalanced braces",
        "function not defined",
        "expected type identifier",
        "excessive nested function calls",
        "return without call",
        "expecting parentheses",
        "expecting while",
        "expecting closing quote",
        "not a string",
        "excessive local variables",
    };
    printf("%s", e[error]);
    p = p_buf;
    while(p != prog) { /*find error line */
        p++;
        if(*p == '\r'){
            linecount++;
        }
    }
    printf(" on line %d\n", linecount);

    temp = p; /* display line containing error */
    for(i=0; i<20 && p>p_buf && *p!='\n'; i++, p--);
    for(i=0; i<30 && p<=temp; i++, p++) printf("%c", *p);

    longjmp(e_buf, 1); /* return to safe pointer */
}

/* Get a token */
int get_token(void)
{
     register char *temp;

     token_type = 0; tok = 0;
     temp = token;
     *temp = '\0';

     /* skip whitespace */
     while(iswhite(*prog) && *prog) ++prog;

     if(*prog=='\r') {
        ++prog;
        ++prog;
            /* skip whitespace */
            while(iswhite(*prog) && *prog) ++prog;
     }

    if(*prog=='\0') { /* block delimiters */
        *token = '\0';
        tok = FINISHED;
    return (token_type = DELIMITER);
    }

    if(strchr("{}", *prog)){  /* block delimiters */
        *temp = *prog;
        temp++;
        *temp = '\0';
        prog++;
    return (token_type = BLOCK);
    }

    /* look for comments */
    if(*prog == '/')
        if(*(prog+1) == '*')  /* it's a comment */
        {
            prog += 2;
            do { /* look for comments */
                while(*prog!='*') prog++;
                    prog++;
                }
                while(*prog!='/');
                prog++;
        }

    if(strchr("!<>=", *prog)) /* is or may be a relational operator */
    {
        switch (*prog)
        {
          case '=':if(*(prog+1)=='=')
            {
                prog++;prog++;
                *temp = EQ;
                temp++; *temp = EQ; temp++;
                *temp = '\0';
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

    if(strchr("+-*^/%=;(),'", *prog)){  /* delimiter */
        *temp = *prog;
        prog++; /* advance to next position */
        temp++;
        *temp = '\0';
    return (token_type = DELIMITER);
    }

    if(*prog=='"') { /* quoted string */
        prog++;
        while(*prog!='"'&& *prog!='\r') *temp++ = *prog++;
        if(*prog=='\r') sntx_err(SYNTAX);
        prog++; *temp = '\0';
    return (token_type = DELIMITER);
    }

    if(isdigit(*prog)) { /* number */
        while(!isdelim(*prog)) *temp++ = *prog++;
        *temp = '\0';
    return(token_type = NUMBER);
    }

    if(isalpha(*prog)) {  /* variable or command */
        while(!isdelim(*prog)) *temp++ = *prog++;
    token_type = TEMP;
    }

    *temp = '\0';

    /* check if a string is a command or a variable */
    if(token_type == TEMP) {
        tok = look_up(token); /* convert to internal representation */
        if(tok) token_type = KEYWORD;
        else token_type = IDENTIFIER;
    }
    return token_type;
 }

/* Return a token to input */
void putback(void)
{
    char *t;

    t = token;
    for(; *t; t++) prog--;
}

/* Look up internal representation of a token in token table. */

int look_up(char *s)
{
    register int i;
    char *p;

    /* convert to lowercase */
    p = s;
    while(*p){ *p = tolower(*p); p++; }

    /* check if token is in table */
    for(i=0; *table[i].command; i++)
        if(!strcmp(table[i].command,s)) return table[i].tok;
    return 0; /* unknown command */
}

/* Return index of internal library function or -1 if not found */
int internal_func(char *s)
{
    int i;

    for(i=0; intern_func[i].f_name[0]; i++){
        if(!strcmp(intern_func[i].f_name, s)) return i;
    }
    return -1;
}

/* Return true if c is a delimiter */
int isdelim(char c)
{
    if(strchr(" !;,+-<>'/*%^=()", c) || c == 9 || c == '\r' || c == 0) return 1;
    return 0;
}

/* Return 1 if c is space or tab */
int iswhite(char c)
{
    if(c==' ' || c== '\t') return 1;
    else return 0;
}
