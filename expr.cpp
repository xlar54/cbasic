#include "stdafx.h"
#include "basic.h"
#include "expr.h"
#include <math.h>

int expr_eval(unsigned char* expr, unsigned char *result, struct Context* ctx)
{
	int error = ERR_NONE;
	unsigned char *postfix = (unsigned char *)malloc(sizeof(unsigned char *) * 255);

	error = expr_infix_to_postfix(expr, postfix, ctx);

	if (error != ERR_NONE)
	{
		free(postfix);
		return error;
	}

	error = expr_eval_postfix(postfix, result, ctx);
	free(postfix);
	return error;
}

int expr_isalnum(int c)
{
	unsigned char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.~abcdefghijklmnopqrstuvwxyz\"$%";
	unsigned char *letter = alphabet;

	while (*letter != '\0' && *letter != c)
		++letter;

	if (*letter)
		return 1;

	return 0;
}

void expr_spush(struct stackdata* data)
{
	int ptr = 0;
	expr_top++;
	strcpy((char *)expr_stack[expr_top].svalue, (char *)data->svalue);
	expr_stack[expr_top].type = data->type;
}

void expr_spop(struct stackdata* data)
{
	if (expr_top == -1) 
	{
		strcpy((char *)data->svalue, "");
		data->type = DATATYPE_UNDEFINED;
	}
	else
	{
		strcpy((char *)data->svalue, (char *)expr_stack[expr_top].svalue);
		data->type = expr_stack[expr_top].type;
		expr_top--;
	}
	return;
}

int expr_priority(unsigned char *x)
{
	unsigned char tkn_and[2];
	unsigned char tkn_or[2];
	
	tkn_and[0] = TOKEN_AND; tkn_and[1] = 0;
	tkn_or[0] = TOKEN_OR; tkn_or[1] = 0;

	if (strcmp((const char*)x, "(") == 0)
		return 0;
	if (strcmp((const char*)x, (const char*)tkn_and) == 0 || strcmp((const char*)x, (const char*)tkn_or) == 0)
		return 1;
	if (strcmp((const char*)x, "=") == 0 || strcmp((const char*)x, ">") == 0 || strcmp((const char*)x, "<") == 0 
		|| strcmp((const char*)x, "<=") == 0 || strcmp((const char*)x, ">=") == 0 || strcmp((const char*)x, "<>") == 0)
		return 2;
	if (strcmp((const char*)x, "+") == 0 || strcmp((const char*)x, "-") == 0)
		return 3;
	if (strcmp((const char*)x, "*") == 0 || strcmp((const char*)x, "/") == 0)
		return 4;
	if (*x > 179)
		return 5;
}

int expr_infix_to_postfix(unsigned char *exp, unsigned char*postfix, struct Context* ctx)
{
	stackdata data;
	//stackdata p;
	unsigned char *e;
	int ptr = 0;
	int ctr = 0;
	int num = 0;
	bool quoteMode = false;

	unsigned char exp2[160];
	e = exp;

	// eat spaces, fix unary, and handle literal strings
	while (*e != '\0')
	{
		if (*e != ' ')
		{
			if (ctr == 0)
			{
				// If + or - is the first op in the expression then its a unary operator
				if (*e == '-' || *e == '+')
				{
					if (*e == '-') exp2[ctr++] = '~';
				}
				else
				{
					exp2[ctr++] = *e;
					if (*e == '\"') quoteMode = !quoteMode;
				}
			}
			else
				if (*e == '+' || *e == '-')
				{
					// If a + or - comes after operator, or comes after a left parenthesis, then it's an unary operator.
					if (exp2[ctr - 1] == '+' || exp2[ctr - 1] == '-' || exp2[ctr - 1] == '*' || exp2[ctr - 1] == '/' || exp2[ctr - 1] == '(')
					{
						if (*e == '-') exp2[ctr++] = '~';
					}
					else
						exp2[ctr++] = *e;
				}
				else
				{
					if (*e == '\"') quoteMode = !quoteMode;
					exp2[ctr++] = *e;
				}
					
		}
		else
		if (quoteMode == true)
		{
			exp2[ctr++] = *e;
			if (*e == '\"')
				quoteMode = false;
		}
		e++;
	}
	
	exp2[ctr] = 0;
	e = exp2;

	while (*e != '\0')
	{
		if (*e == ',')
		{
			e++;
			continue;
		}
			

		// add literal strings to the postfix expr
		if (*e == '\"')
		{
			postfix[ptr++] = *(e++);
			while (*e != '\"' && *e != 0)
				postfix[ptr++] = *(e++);

			postfix[ptr++] = '\"';
			postfix[ptr++] = ',';
		}
		else
		if (expr_isalnum(*e))
		{
			while (expr_isalnum(*e))
			{
				if (*e == '~' && *(e + 1) == '(')
				{
					postfix[ptr++] = '0';
					
					data.svalue[0] = '-';
					data.svalue[1] = 0;
					data.type = 0;
					expr_spush(&data);
					e++;
					break;
				}
				postfix[ptr++] = *(e++);
			}

			postfix[ptr++] = ',';
			e--;
		}
		else if (*e == '(')
		{
			data.svalue[0] = *e;
			data.svalue[1] = 0;
			data.type = 0;
			expr_spush(&data);
		}
		else if (*e == ')')
		{
			while (1)
			{
				expr_spop(&data);

				if (data.type == -1)
				{
					return ERR_SYNTAX;
				}

				if (strcmp((const char*)data.svalue, "(") != 0)
				{
					int tptr = 0;
					while (data.svalue[tptr] != 0)
						postfix[ptr++] = data.svalue[tptr++];

					postfix[ptr++] = ',';
				}
				else
					break;
			}
		}
		else
		{
			if (*e == '>' && *(e + 1) == '=')
			{
				data.svalue[0] = '>'; data.svalue[1] = '=';  data.svalue[2] = 0; e++;
			}
			else
			if (*e == '<' && *(e + 1) == '=')
			{
				data.svalue[0] = '<'; data.svalue[1] = '=';  data.svalue[2] = 0; e++;
			}
			else
			if (*e == '<' && *(e + 1) == '>')
			{
				data.svalue[0] = '<'; data.svalue[1] = '>';  data.svalue[2] = 0; e++;
			}
			else
			{
				data.svalue[0] = *e;
				data.svalue[1] = 0;
			}
//printf("\n%d >= %d", expr_priority(expr_stack[expr_top]), expr_priority(tkn));
			while (expr_top != -1 && expr_priority(expr_stack[expr_top].svalue) >= expr_priority(data.svalue))
			{
				expr_spop(&data);
				//if (data.type == -1)
				//	break;

				int tptr = 0;
				while (data.svalue[tptr] != 0)
					postfix[ptr++] = data.svalue[tptr++];

				postfix[ptr++] = ',';
				data.svalue[0] = *e;
				data.svalue[1] = 0;
			}


			data.type = 0;
			expr_spush(&data);
		}
		e++;
	}
	while (expr_top != -1)
	{
		expr_spop(&data);
		int tptr = 0;
		while (data.svalue[tptr] != 0)
			postfix[ptr++] = data.svalue[tptr++];
	}

	postfix[ptr] = 0;

	return ERR_NONE;
}

int expr_eval_postfix(unsigned char* postfix, unsigned char* result, struct Context* ctx)
{
	stackdata data;
	unsigned char sn1[255];
	unsigned char sn2[255];
	unsigned char sn3[255];

	int optyp1, optyp2, optyp3 = 0;

	double n1 = 0;
	double n2 = 0;
	double n3 = 0;
	int ctr = 0, ptr=0;
	unsigned char *p, *e;
	char str_value[10];
	bool negateVar = false;

//printf("\n\n%s,%d", postfix, strlen((const char *)postfix));
	e = postfix;
	
	while (*e != '\0')
	{
		ctr = 0;

		if (*e == ',')
		{
			e++;
			continue;
		}

		// digit?
		if ((*e > 47 && *e < 58) || *e == '~' || *e == '.')
		{
			while ((*e > 47 && *e < 58) || *e == '~' || *e == '.')
			{
				if (*e == '~')
				{
					if (ISALPHA(*(e + 1)))
						negateVar = true;
					else
						*e = '-';
				}
				
				data.svalue[ctr++] = *(e++);
			}
			data.svalue[ctr] = 0;
			data.type = DATATYPE_FLOAT;
			expr_spush(&data);
			continue;
		}

		// string literal?
		if (*e == '\"')
		{
			e++;
			while (*e != '\"' && *e != 0)
			{
				data.svalue[ctr++] = *(e++);
			}
			
			if (*e == '\"') e++;

			data.svalue[ctr] = 0;
			data.type = DATATYPE_STRING;
			expr_spush(&data);
			continue;
		}

		// variable?
		int varFound = 0;
		int ctr2 = 0;

		if (ISALPHA(e[0]))
		{
			unsigned char tkn[10] = { 0 };

			while (*e != ',' && *e != 0)
			{
				tkn[ctr2++] = *(e++);
			}
			tkn[ctr2] = 0;

			for (int j = 0; j < ctx->var_count; j++)
			{
				if (compare(ctx->vars[j].name, tkn))
				{
					if (ctx->vars[j].type == DATATYPE_STRING)
					{
						strcpy((char *)data.svalue, (char *)((char*)ctx->vars[j].location));
						data.type = DATATYPE_STRING;
						expr_spush(&data);
					}
						
					else
					if (ctx->vars[j].type == DATATYPE_FLOAT)
					{
						double value = *((float*)ctx->vars[j].location);
						if (negateVar) value = -value;
						snprintf(str_value, 10, "%f", value);
						strcpy((char *)data.svalue, (char*)str_value);
						data.type = DATATYPE_FLOAT;
						expr_spush(&data);
					}
					else
					if (ctx->vars[j].type == DATATYPE_INT)
					{
						int value = *((int*)ctx->vars[j].location);
						if (negateVar) value = -value;
						snprintf(str_value, 10, "%d", value);
						strcpy((char *)data.svalue, (char*)str_value);
						data.type = DATATYPE_INT;
						expr_spush(&data);
					};
					
					varFound = 1;
					break;
				}
			}

			if (varFound == 0)
			{
				if (tkn[length(tkn) - 1] == '$')
				{
					var_add_update_string(ctx, tkn, (unsigned char *)"\0", 1);
					strcpy((char *)data.svalue, "");
					data.type = DATATYPE_STRING;
					expr_spush(&data);
				}
				else
					if (tkn[length(tkn) - 1] == '%')
					{
						var_add_update_int(ctx, tkn, 0);
						strcpy((char *)data.svalue, "0");
						data.type = DATATYPE_INT;
						expr_spush(&data);
					}
					else
					{
						var_add_update_float(ctx, tkn, 0);
						strcpy((char *)data.svalue, "0");
						data.type = DATATYPE_FLOAT;
						expr_spush(&data);
					}
			}

			negateVar = false;
			continue;
		}


		expr_spop(&data);

		if (data.type == DATATYPE_UNDEFINED)
			return ERR_SYNTAX;

		strcpy((char *)sn1, (char *)data.svalue);
		optyp1 = data.type;

		// functions
		if (*e == TOKEN_ABS)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n3 = atof((const char*)sn1) < 0 ? atof((const char*)sn1) * -1 : atof((const char*)sn1);
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_ASC)
		{
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n3 = sn1[0];
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_ATN)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			n3 = atan(n1);

			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_CHR$)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			sn3[0] = atoi((char *)sn1);
			sn3[1] = 0;
			optyp3 = DATATYPE_STRING;
		}
		else
		if (*e == TOKEN_COS)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			n3 = cos(n1);

			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_EXP)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			n3 = exp(n1);

			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_INT)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n3 = atof((const char*)sn1);
			optyp3 = DATATYPE_INT;
		}
		else
		if (*e == TOKEN_LEFT$)
		{
			if (data.type != DATATYPE_FLOAT && data.type != DATATYPE_INT)
				return ERR_TYPE_MISMATCH;

			n1 = atoi((const char*)sn1);

			if (n1 < 1)
				return ERR_ILLEGAL_QTY;

			expr_spop(&data);
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;
			
			int x = 0;
			for(; x < n1; x++)
			{
				if (data.svalue[x] == 0)
					break;

				sn3[x] = data.svalue[x];
			}
			sn3[x] = 0;
			optyp3 = DATATYPE_STRING;

		}
		else
		if (*e == TOKEN_LEN)
		{
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n3 = strlen((char *)sn1);
			optyp3 = DATATYPE_INT;
		}
		else
		if (*e == TOKEN_LOG)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);

			if (n1 <= 0)
				return ERR_ILLEGAL_QTY;

			n3 = log(n1);
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_MID$)
		{
			if (data.type != DATATYPE_FLOAT && data.type != DATATYPE_INT)
				return ERR_TYPE_MISMATCH;

			n1 = atoi((const char*)sn1);

			expr_spop(&data);
			if (data.type != DATATYPE_FLOAT && data.type != DATATYPE_INT)
				return ERR_TYPE_MISMATCH;

			n2 = atoi((const char*)data.svalue);

			if(n1 < 1 || n2 < 1)
				return ERR_ILLEGAL_QTY;

			expr_spop(&data);
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			int x = n2-1;
			int y = 0;
			for (; y < n1; y++)
			{
				if (data.svalue[x] == 0)
					break;

				sn3[y] = data.svalue[x++];
			}
			sn3[y] = 0;
			optyp3 = DATATYPE_STRING;

		}
		else
		if (*e == TOKEN_RIGHT$)
		{
			if (data.type != DATATYPE_FLOAT && data.type != DATATYPE_INT)
				return ERR_TYPE_MISMATCH;

			n1 = atoi((const char*)sn1);

			if (n1 < 1)
				return ERR_ILLEGAL_QTY;

			optyp3 = DATATYPE_INT;

			expr_spop(&data);
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = strlen((char *)data.svalue) - n1;
			if (n1 < 0)
				n1 = 0;

			int x = 0;
			while(data.svalue[(int)n1] != 0)
				sn3[x++] = data.svalue[(int)n1++];

			sn3[x] = 0;
			optyp3 = DATATYPE_STRING;

		}
		else
		if (*e == TOKEN_RND)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			// in real cbm basic, the param actually means something

			n3 = (double)rand() / (double)RAND_MAX;
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_SGN)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			if (atof((const char*)sn1) < 0) n3 = -1;
			if (atof((const char*)sn1) == 0) n3 = 0;
			if (atof((const char*)sn1) > 0) n3 = 1;
			
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_SIN)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			n3 = sin(n1);

			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_SQR)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			
			if (n1 < 0)
				return ERR_ILLEGAL_QTY;

			n3 = sqrt(n1);
			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_STR$)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			int x = 0;
			for (; x < length(sn1); x++)
			{
				if (ISDIGIT(sn1[x]) || sn1[x] == '.')
					sn3[x] = sn1[x];
				else
					break;
			}
			sn3[x] = 0;
			optyp3 = DATATYPE_STRING;
		}
		else
		if (*e == TOKEN_TAN)
		{
			if (data.type == DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			n1 = atof((const char*)sn1);
			n3 = tan(n1);

			optyp3 = DATATYPE_FLOAT;
		}
		else
		if (*e == TOKEN_VAL)
		{
			if (data.type != DATATYPE_STRING)
				return ERR_TYPE_MISMATCH;

			for (int x = 0; x < length(sn1); x++)
			{
				if (ISDIGIT(sn1[x]) || sn1[x] == '.')
					sn3[x] = sn1[x];
				else
				{
					sn3[x] = 0;
					break;
				}	
			}

			n3 = atof((const char*)sn3);
			optyp3 = DATATYPE_FLOAT;
		}
		else
		{
			expr_spop(&data);
			if (data.type == DATATYPE_UNDEFINED)
			{
				return ERR_SYNTAX;
			}

			strcpy((char *)sn2, (char *)data.svalue);
			optyp2 = data.type;

			if (*e == '>' && *(e + 1) == '=')
			{
				n3 = -(atof((const char*)sn2) >= atof((const char*)sn1));
				e++;
			}
			else
			if (*e == '<' && *(e + 1) == '=')
			{
				n3 = -(atof((const char*)sn2) <= atof((const char*)sn1));
				e++;
			}
			else
			if (*e == '<' && *(e + 1) == '>')
			{
				n3 = -(atof((const char*)sn2) != atof((const char*)sn1));
				e++;
			}
			else
			{
				if (optyp1 != DATATYPE_STRING && optyp2 != DATATYPE_STRING)
				{
					optyp3 = DATATYPE_FLOAT;

					switch (*e)
					{
						case TOKEN_AND: { n3 = -(atof((const char*)sn2) && atof((const char*)sn1)); break; }
						case TOKEN_OR: { n3 = -(atof((const char*)sn2) || atof((const char*)sn1)); break; }
						case '+': { n3 = atof((const char*)sn2) + atof((const char*)sn1); break; }
						case '-': { n3 = atof((const char*)sn2) - atof((const char*)sn1); break; }
						case '*': { n3 = atof((const char*)sn2) * atof((const char*)sn1); break; }
						case '/':
						{
							if (atof((const char*)sn1) == 0)
							{
								return ERR_DIV0;
							}
							n3 = atof((const char*)sn2) / atof((const char*)sn1);
							break;
						}
						case '=': { n3 = -(atof((const char*)sn2) == atof((const char*)sn1));	break; }
						case '>': {	n3 = -(atof((const char*)sn2) > atof((const char*)sn1));	break; }
						case '<': { n3 = -(atof((const char*)sn2) < atof((const char*)sn1));	break; }
					}

					
				}
				else
				if (optyp1 == DATATYPE_STRING && optyp2 == DATATYPE_STRING)
				{
					optyp3 = DATATYPE_STRING;

					if (*e == '+')
					{
						strcpy((char *)sn3, (char *)sn2);
						strcat((char *)sn3, (char *)sn1);
					}
					else
						if (*e == '=')
						{
							optyp3 = DATATYPE_INT;
							n3 = ((strcmp((char *)sn1, (char *)sn2) == 0) ? -1 : 0);
						}
						else
							return ERR_TYPE_MISMATCH;

				}
				else
				{
					return ERR_TYPE_MISMATCH;
				}
			}
		}

		if(optyp3 == DATATYPE_FLOAT)
			snprintf((char*)sn3, 20, "%f", n3);

		if (optyp3 == DATATYPE_INT)
			snprintf((char*)sn3, 10, "%d", (int)n3);

		strcpy((char *)data.svalue, (char *)sn3);
		data.type = optyp3;
		expr_spush(&data);

		e++;
	}
	
	expr_spop(&data);
	ctx->expression_type = data.type;
	strcpy((char *)result, (char *)data.svalue);
	return ERR_NONE;
	
}
