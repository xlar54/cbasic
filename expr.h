#ifndef _EXPR_H_
#define _EXPR_H

extern "C"
{
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

struct stackdata {
	unsigned char svalue[200];
	int type;
};

typedef stackdata ExpessionStack;
static ExpessionStack expr_stack[10];

//static unsigned char expr_stack[200][10];
static int expr_type_stack[10];
static int expr_top = -1;
static int expr_type = 0;

int expr_eval(unsigned char* expr, unsigned char *result, struct Context* ctx);
int expr_infix_to_postfix(unsigned char *exp, unsigned char*postfix, struct Context* ctx);
int expr_eval_postfix(unsigned char* postfix, unsigned char* result, struct Context* ctx);
int expr_isalnum(int c);
void expr_spush(struct stackdata* data);
void expr_spop(struct stackdata* data);
int expr_priority(unsigned char *x);


}



#endif