/*   Internal library functions   */
/* add more here, or in other words, put your account at risk  */
//#include<conio.h>       /* Remove this if your compiler doesn't support this header file */
#include<stdio.h>
#include<stdlib.h>

extern char *prog;      /* points to the current program position */
extern char token[80];  /* holds the string representation of the token  */
extern char token_type; /* contains the token type */
extern char tok;        /* holds the internal representation of the token */

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK};

/* These are the constants used to call sntx_err() when a syntax error occurs. Add more if desired.
*  SYNTAX is a generic error message used when no other seems appropriate.*/
enum error_msg
{
    SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
    NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
    UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
    NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
    WHILE_EXPECTED, QUOTE_EXPECTED, NOT_STRING, TOO_MANY_LVARS
};

int get_token(void);
void sntx_err(int error), eval_exp(int *result);
void putback(void);

/* Gets a character from console, use getchar() if your compiler doesn't support getche() */
int call_getche()
{
    char ch;
    ch = getchar();
    while (*prog!=')') prog++;
    prog++;     /* advances to the end of the line */
    return ch;
}

/* displays a character on the screen */
int call_putch()
{
    int value;
    eval_exp(&value);
    printf("%c", value);
    return value;
}

/* calls puts */
int call_puts(void)
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

/* a built-in output function for control */
int print(void)
{
    int i;
    get_token();
    if(*token!='(') sntx_err(PAREN_EXPECTED);
    get_token();
    if(token_type==STRING) {    /* displays a string */
        printf("%s ", token);
    }
     else {    /* a number */
        putback();
        eval_exp(&i);
        printf("%d ", i);
    }
    get_token();
    if(*token!=')') sntx_err(PAREN_EXPECTED);
    get_token();
    if(*token!=';') sntx_err(SEMI_EXPECTED);
    putback();
    return 0;
}

/* Reads an int from the keyboard */
int getnum(void)
{
    char s[80];
    gets(s);
    while(*prog!=')') prog++;
    prog++;   /* advances to the end of the line */
    return atoi(s);
}
