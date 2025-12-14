/*---------------------------------------------
 *  Mini C Interpreter - 0.2
 *  Author: Cristiano Camacho
 *  Date:   03/17/2022 to 04/19/2022
 *  Desc:   Interpreter in C, implementing a
 *  subset of C, more details in readme
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

enum tok_types { DELIMITER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK };

/* Add other C tokens here */
enum tokens { ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE, SWITCH, RETURN, EOL, FINISHED, END };

/* Add other double operators (such as ->) here */
enum double_ops { LT=1, LE, GT, GE, EQ, NE };

/* These are the constants used to call sntx_err() when a syntax error occurs. Add more if desired.
*  SYNTAX is a generic error message used when no other seems appropriate.*/

enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED, NOT_VAR, PARAM_ERR, SEMI_EXPECTED, UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
    NEST_FUNC, RET_NOCALL, PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP, TOO_MANY_LVARS
};

char *prog;  /* current position in source code */
char *p_buf; /* points to the beginning of program load area */

jmp_buf e_buf; /* holds environment for longjmp() */

/* An array of these structures will hold the information associated with global variables */

struct var_type
{
    char var_name[ID_LEN];
    int var_type;
    int value;
} global_vars[NUM_GLOBAL_VARS];

struct var_type local_var_stack[NUM_LOCAL_VARS];

/* This is the function call stack */
struct func_type
{
    char func_name[ID_LEN];
    char *loc; /* position of function entry point in file */
} func_table[NUM_FUNC];

int call_stack[NUM_FUNC];

/* Keyword lookup table */
struct commands
{
    char command[20];
    char tok;
} table[] = {   /* commands must be written in lowercase */
    "if", IF,
    "else", ELSE,
    "for", FOR,
    "do", DO,
    "while", WHILE,
    "char", CHAR,
    "int", INT,
    "return", RETURN,
    "end", END,
    "", END /* marks end of table */
};

char token[80];
char token_type, tok;

int functos; /* index to top of function call stack */

int func_index;  /* index in function table */
int gvar_index;  /* index in global variable table */
int lvartos;     /* index to local variable stack */

int ret_value;   /* function return value */

void print(void), prescan(void);
void decl_global(void), call(void), putback(void);
void decl_local(void), local_push(struct var_type i);
void eval_exp(int *value), sntx_err(int error);
void exec_if(void), find_eob(void), exec_for(void);
void get_params(void), get_args(void);
void exec_while(void), exec_do(void), func_push(int i);
void assign_var(char *var_name, int value);
int load_program(char *p, char *fname), find_var(char *s);
void interp_block(void), func_ret(void);
int func_pop(void), is_var(char *s), get_token(void);

char *find_func(char *name);

int main(int argc, char **argv)
{
    if(argc!=2) {
        printf("Usage: minic <filename>\n");
        exit(1);
    }

    /* allocate memory for the program */
    if((p_buf=(char *) malloc(PROG_SIZE))==NULL){
        printf("Allocation failure");
        exit(1);
    }

    /* load the program to execute */
    if(!load_program(p_buf, argv[1])) exit(1);
    if(setjmp(e_buf)) exit(1); /* initialize long jump buffer */

    gvar_index = 0; /* initialize global variable index */
    /* set program pointer to beginning of buffer */
    prog = p_buf;

    prescan(); /* find position of all functions and global variables in program */

    lvartos = 0;    /* initialize local variable index */
    functos = 0;    /* initialize CALL stack index */
    /* prepare main() call */
    prog = find_func("main"); /* find program start point */
    prog--; /* go back to initial ( */
    strcpy(token, "main");

    call(); /* start interpreting main() */
    return 0;
}

/* Interprets a single statement or code block. When interp_block() returns from its initial call,
   the final brace (or a return) was found in main().
*/

void interp_block(void)
{
    int value;
    char block = 0;

    do {
        token_type = get_token();

        /* If interpreting a single statement, return when finding the first ;    */
        /* see what type of token is ready */
        if(token_type==IDENTIFIER) {
            /* not a keyword, so process expression */
            putback(); /* return token to input for later processing by eval_exp() */
            eval_exp(&value); /* process the expression */
            if(*token!=';') sntx_err(SEMI_EXPECTED);
        }
        else if(token_type==BLOCK) { /* if block delimiter... */
                if(*token=='{') /* if equal to a block*/
                    block = 1;  /* interpreting a block not a statement */
                else return;    /* it's a }, so don't return */
        }
        else /* it's a keyword */
        switch(tok) {
            case CHAR:
            case INT:       /* declare local variables */
                putback();
                decl_local();
                break;
            case RETURN:    /* return from function call */
                func_ret();
                return;
            case IF:        /* process an if statement */
                exec_if();
                break;
            case ELSE:      /* process an else statement*/
                find_eob(); /* find end of else block and continue execution */
                break;
            case WHILE:     /* process a while loop */
                exec_while();
                break;
            case DO:        /* process a do-while loop */
                exec_do();
                break;
            case FOR:       /* process a for loop */
                exec_for();
                break;
            case END:
                exit(0);
        }
    } while(tok != FINISHED && block);
}

/* load a program */
int load_program(char *p, char *fname)
{
    FILE *fp;
    int i = 0;
    if((fp=fopen(fname, "rb"))==NULL) return 0;

    i = 0;
    do {
        *p = getc(fp);
        p++; i++;
    } while(!feof(fp) && i<PROG_SIZE);
    if(*(p-2)==0x1a) *(p-2) = '\0'; /* terminate program with null */

    else *(p-1) = '\0';
    fclose(fp);
    return 1;
}

/* Find position of all functions in program and store all global variables */
void prescan(void)
{
    char *p;
    char temp[32];
    int brace = 0; /* When 0, this variable indicates that current position in source code
    is outside of any function */

    p = prog;
    func_index = 0;
    do {
        while(brace) { /* skip code inside functions */
                get_token();
                if(*token=='{') brace++;
                if(*token=='}') brace--;
        }
        get_token();

        if(tok==CHAR || tok==INT) {     /* it's a global variable */
            putback();
            decl_global();
        }
        else if(token_type==IDENTIFIER) {
            strcpy(temp, token);
            get_token();
            if(*token=='(') {   /* must be a function */
                func_table[func_index].loc = prog;
                strcpy(func_table[func_index].func_name, temp);
                func_index++;
                while(*prog!=')') prog++;
                prog++;
                /* now prog points to opening brace of function */
               }
               else putback();
        }
        else if(*token=='{') brace++;
    }while(tok!=FINISHED);
    prog = p;
  }

/* returns entry point of specified function, returns NULL if not found */
char *find_func(char *name)
{
    register int i;

    for(i=0; i<func_index; i++)
        if(!strcmp(name, func_table[i].func_name))
            return func_table[i].loc;
    return NULL;
}

/* declare a global variable */
void decl_global(void)
{
    int vartype;

    get_token(); /* get type */

    vartype = tok;

    do{     /* process comma-separated list */
        global_vars[gvar_index].var_type = vartype;
        global_vars[gvar_index].value = 0; /* initialize with zero */
        get_token();  /* get name */
        strcpy(global_vars[gvar_index].var_name, token);
        get_token();
        gvar_index++;
    }while(*token==',');
    if(*token!=';') sntx_err(SEMI_EXPECTED);
}

/* declare a local variable */
void decl_local(void)
{
    struct var_type i;

    get_token();   /* get type */

    i.var_type = tok;
    i.value = 0; /* initialize with zero */

    do{     /* process comma-separated list */
        get_token();    /* get variable name */
        strcpy(i.var_name, token);
        local_push(i);
        get_token();
    }while(*token==',');
        if(*token!=';') sntx_err(SEMI_EXPECTED);
}

/* call a function */
void call(void)
{
    char *loc, *temp;
    int lvartemp;

    loc = find_func(token);  /* find function entry point */

    if(loc==NULL)
        sntx_err(FUNC_UNDEF); /* function not defined */
    else {
        lvartemp = lvartos; /* save local var stack index */

        get_args();  /* get function arguments */
        temp = prog; /* save return address*/
        func_push(lvartemp); /* save local var stack index */

        prog = loc; /* reset prog to start of function */
        get_params(); /* load function parameters with argument values */

        interp_block(); /* interpret the function */
        prog = temp;    /* reset program pointer */
        lvartos = func_pop();  /* reset local var stack */
    }
}

/* push function arguments onto local variable stack */
void get_args(void)
{
    int value, count, temp[NUM_PARAMS];
    struct var_type i;

    count = 0;
    get_token();
    if(*token!='(') sntx_err(PAREN_EXPECTED);

    /* process comma-separated list of values */
    do {
        eval_exp(&value);
        temp[count] = value;  /* temporarily save */
        get_token();
        count++;
    }while(*token==',');
    count--;
    /* now, push onto local_var_stack in reverse order */
    for(; count>=0; count--) {
        i.value = temp[count];
        i.var_type = ARG;
        local_push(i);
    }
}

/* get function parameters */
void get_params(void)
{
    struct var_type *p;
    int i;

    i=lvartos-1;
    do{     /* process comma-separated parameter list */
        get_token();
        p = &local_var_stack[i];
        if(*token!=')') {
            if(tok!=INT && tok!= CHAR) sntx_err(TYPE_EXPECTED);
            p->var_type = token_type;
            get_token();

            /* bind parameter name with argument already on local variable stack */
            strcpy(p->var_name, token);
            get_token();
            i--;
        }
        else break;
    }   while (*token==',');
    if(*token!=')') sntx_err(PAREN_EXPECTED);
}

/* return from a function */
void func_ret(void)
{
    int value;

    value = 0;
    /* get return value, if any */
    eval_exp(&value);

    ret_value = value;
}

/* Push a local variable */
void local_push(struct var_type i)
{
    if(lvartos>NUM_LOCAL_VARS)
        sntx_err(TOO_MANY_LVARS);

    local_var_stack[lvartos] = i;
    lvartos++;
}

/* Pop index from local variable stack */
func_pop(void)
{
    functos--;
    if(functos<0) sntx_err(RET_NOCALL);
        return(call_stack[functos]);
}

/* Push local variable stack index */
void func_push(int i)
{
    if(functos>NUM_FUNC)
        sntx_err(NEST_FUNC);
    call_stack[functos] = i;
    functos++;
}

/* Assign a value to a variable */
void assign_var(char *var_name, int value)
{
    register int i;

    /* first, see if it's a local variable */
    for(i=lvartos-1; i>=call_stack[functos-1]; i--) {
        if(!strcmp(local_var_stack[i].var_name, var_name)) {
            local_var_stack[i].value = value;
            return;
        }
    }
    if(i < call_stack[functos-1])
        /* if not local, try global variable table */
        for(i=0; i<NUM_GLOBAL_VARS; i++)
            if(!strcmp(global_vars[i].var_name, var_name)) {
                global_vars[i].value = value;
                return;
            }
        sntx_err(NOT_VAR);   /* variable not found */
}

/* find the value of a variable */
int find_var(char *s)
{
    register int i;

    /* first, see if it's a local variable */
    for(i=lvartos-1; i>=call_stack[functos-1]; i--)
        if(!strcmp(local_var_stack[i].var_name, token))
            return local_var_stack[i].value;

    /* if not local, try global variable table */
    for(i=0; i<NUM_GLOBAL_VARS; i++)
        if(!strcmp(global_vars[i].var_name, s))
            return global_vars[i].value;

    sntx_err(NOT_VAR);   /* variable not found */
}

/* Determine if an identifier is a variable.
   Returns 1 if variable is found. 0 otherwise.
*/
int is_var(char *s)
{
    register int i;

    /* first, see if it's a local variable */
    for(i=lvartos-1; i>=call_stack[functos-1]; i--)
        if(!strcmp(local_var_stack[i].var_name, token))
            return 1;

    /* if not local, try global variables */
    for(i=0; i<NUM_GLOBAL_VARS; i++)
        if(!strcmp(global_vars[i].var_name, s))
            return 1;

    return 0;
}

/* Execute an IF statement */
void exec_if(void)
{
    int cond;
    eval_exp(&cond); /* get left expression */

    if(cond) {  /* it's true, so process IF target */
        interp_block();
    }
    else{   /* otherwise, skip IF block and process ELSE, if it exists */
        find_eob();  /* find end of next line */
        get_token();

        if(tok!=ELSE) {
            putback();  /* return token if not ELSE */
            return;
        }
        interp_block();
    }
}

/* Execute a while loop */
void exec_while(void)
{
    int cond;
    char *temp;

    putback();
    temp = prog;  /* save position of while loop start */
    get_token();
    eval_exp(&cond);    /* check conditional expression */
    if(cond) interp_block();  /* if true, interpret */
        else{   /* otherwise, skip loop */
            find_eob();
            return;
        }
        prog = temp; /* return to loop start */
}

/* Execute a DO loop */
void exec_do(void)
{
    int cond;
    char *temp;

    putback();
    temp = prog; /* save position of do loop start */
    get_token(); /* get loop start */
    interp_block(); /* interpret loop */
    get_token();
    if(tok!=WHILE) sntx_err(WHILE_EXPECTED);
    eval_exp(&cond); /* check loop condition */
    if(cond) prog = temp; /* if true, repeat; otherwise, continue */
}

/* Find end of block */
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

/* Execute a for loop */
void exec_for(void)
{
    int cond;
    char *temp, *temp2;
    int brace;

    get_token();
    eval_exp(&cond); /* initialization expression */
    if(*token!=';') sntx_err(SEMI_EXPECTED);
    prog++;
    temp = prog;
    for(;;) {
        eval_exp(&cond); /* check condition */
        if(*token!=';') sntx_err(SEMI_EXPECTED);
        prog++;     /* pass the ; */
        temp2 = prog;

        /* Find start of for block */
        brace = 1;
        while(brace) {
            get_token();
            if(*token=='(') brace++;
            if(*token==')') brace--;
        }
        if(cond) interp_block();   /* if true, interpret */
            else{                  /* otherwise, skip loop */
                find_eob();
                return;
            }
            prog = temp2;
            eval_exp(&cond);        /* perform increment */
            prog = temp;            /* return to start */
    }
}
